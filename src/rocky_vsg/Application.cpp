/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "Application.h"
#include "MapNode.h"
#include "TerrainNode.h"
#include "MapManipulator.h"
#include "SkyNode.h"
#include <rocky/Ephemeris.h>

#include <vsg/app/CloseHandler.h>
#include <vsg/app/View.h>
#include <vsg/app/RenderGraph.h>
#include <vsg/utils/CommandLine.h>
#include <vsg/nodes/Light.h>

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::engine;

Application::Application(int& argc, char** argv) :
    instance()
{
    vsg::CommandLine commandLine(&argc, argv);

    commandLine.read(instance._impl->runtime.readerWriterOptions);
    _debuglayer = commandLine.read({ "--debug" });
    _apilayer = commandLine.read({ "--api" });
    _vsync = !commandLine.read({ "--novsync" });

    viewer = vsg::Viewer::create();

    root = vsg::Group::create();

    mainScene = vsg::Group::create();

    root->addChild(mainScene);

    mapNode = rocky::MapNode::create(instance);

    // the sun
    if (commandLine.read({ "--sky" }))
    {
        auto sky = rocky::SkyNode::create(instance);
        mainScene->addChild(sky);
    }

    mapNode->terrainNode()->concurrency = 4u;
    mapNode->terrainNode()->skirtRatio = 0.025f; 
    mapNode->terrainNode()->minLevelOfDetail = 1;
    mapNode->terrainNode()->screenSpaceError = 135.0f;

    // wireframe overlay
    if (commandLine.read({ "--wire" }))
        instance.runtime().shaderCompileSettings->defines.insert("RK_WIREFRAME_OVERLAY");

    mainScene->addChild(mapNode);

    // Set up the runtime context with everything we need.
    // Eventually this should be automatic in InstanceVSG
    instance.runtime().compiler = [this]() { return viewer->compileManager; };
    instance.runtime().updates = [this]() { return viewer->updateOperations; };
    instance.runtime().sharedObjects = vsg::SharedObjects::create();
}

void
Application::createMainWindow(int width, int height, const std::string& name)
{
    auto traits = vsg::WindowTraits::create(name);
    traits->debugLayer = _debuglayer;
    traits->apiDumpLayer = _apilayer;
    traits->samples = 1;
    traits->width = 1920;
    traits->height = 1080;
    if (!_vsync)
        traits->swapchainPreferences.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    mainWindow = vsg::Window::create(traits);
    mainWindow->clearColor() = VkClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
    viewer->addWindow(mainWindow);
}

std::shared_ptr<Map>
Application::map()
{
    return mapNode->map();
}

int
Application::run()
{
    // Make a window if the user didn't.
    if (!mainWindow)
    {
        createMainWindow(1920, 1080);
    }

    // main camera
    double nearFarRatio = 0.00001;
    double R = mapNode->mapSRS().ellipsoid().semiMajorAxis();

    auto perspective = vsg::Perspective::create(
        30.0,
        (double)(mainWindow->extent2D().width) / (double)(mainWindow->extent2D().height),
        R * nearFarRatio,
        R * 10.0);

    auto camera = vsg::Camera::create(
        perspective,
        vsg::LookAt::create(),
        vsg::ViewportState::create(mainWindow->extent2D()));

    viewer->addEventHandler(vsg::CloseHandler::create(viewer));

    viewer->addEventHandler(rocky::MapManipulator::create(mapNode, camera));

    // View pairs a camera with a scene graph and manages
    // view-dependent state like lights and viewport.
    auto view = vsg::View::create(camera);
    view->addChild(root);
    
    // RenderGraph encapsulates vkCmdRenderPass/vkCmdEndRenderPass and owns things
    // like the clear color, render area, and a render target (framebuffer or window).
    auto renderGraph = vsg::RenderGraph::create(mainWindow, view);

    // CommandGraph holds the command buffers that the vk record/submit task
    // will use during record traversal.
    auto commandGraph = vsg::CommandGraph::create(mainWindow);
    commandGraph->addChild(renderGraph);

    // This sets up the internal tasks that will, for each command graph, record
    // a scene graph and submit the results to the renderer each frame. Also sets
    // up whatever's necessary to present the resulting swapchain to the device.
    viewer->assignRecordAndSubmitTaskAndPresentation({ commandGraph });

    // Configure a descriptor pool size that's appropriate for paged terrains
    // (they are a good candidate for DS reuse). This is optional.
    // https://groups.google.com/g/vsg-users/c/JJQZ-RN7jC0/m/tyX8nT39BAAJ
    auto resourceHints = vsg::ResourceHints::create();
    resourceHints->numDescriptorSets = 1024;
    resourceHints->descriptorPoolSizes.push_back(
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 });

    // Initialize and compile existing any Vulkan objects found in the scene
    // (passing in ResourceHints to guide the resources allocated).
    viewer->compile(resourceHints);

    // Use a separate thread for each CommandGraph?
    // https://groups.google.com/g/vsg-users/c/-YRI0AxPGDQ/m/A2EDd5T0BgAJ
    viewer->setupThreading();

    // The main frame loop
    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();

        // since an event handler could deactivate the viewer:
        if (!viewer->active())
            break;

        // rocky update pass - management of tiles and paged data
        mapNode->update(viewer->getFrameStamp());

        // user's update function
        if (updateFunction)
        {
            updateFunction();
        }

        // run through the viewer's update operations queue; this includes update ops 
        // initialized by rocky (tile merges or MapObject adds)
        viewer->update();

        // handle any object additions.
        processAdditions();

        viewer->recordAndSubmit();

        viewer->present();
    }

    return 0;
}

void
Application::processAdditions()
{
    // Any new nodes in the scene? integrate them now.
    if (!_additions.empty())
    {
        std::scoped_lock(_additions_mutex);
        while (!_additions.empty())
        {
            auto& a = _additions.front();

            mainScene->addChild(a.node);

            // if it didn't compile earlier, compile it now:
            if (!a.compileResult)
                a.compileResult = viewer->compileManager->compile(a.node);

            // and integrate it into the viewer:
            if (a.compileResult)
                vsg::updateViewer(*viewer, a.compileResult);

            _additions.pop();
        }
    }
}

void
Application::add(shared_ptr<MapObject> obj)
{
    ROCKY_SOFT_ASSERT_AND_RETURN(obj, void());

    std::vector<Addition> additions;

    for (auto& attachment : obj->attachments)
    {
        Addition addition;
        addition.node = attachment->getOrCreateNode(instance.runtime());
        if (addition.node)
        {
            // try to compile it now; if there's no compile manager yet, we will compile it
            // on demand later in processAdditions.
            if (viewer->compileManager)
            {
                addition.compileResult = viewer->compileManager->compile(addition.node);
            }
            additions.emplace_back(std::move(addition));
        }
    }

    if (!additions.empty())
    {
        std::scoped_lock(_additions_mutex);
        for (auto& a : additions)
            _additions.push(std::move(a));
    }
}



MapObject::MapObject(shared_ptr<Attachment> value) :
    super()
{
    attachments.push_back(value);
}

MapObject::MapObject(Attachments value) :
    super()
{
    attachments = value;
}


LineString::LineString() :
    super()
{
    _geometry = LineStringGeometry::create();
    _bindStyle = BindLineStyle::create();
}

void
LineString::pushVertex(float x, float y, float z)
{
    _geometry->push_back({ x, y, z });
}

void
LineString::setStyle(const LineStyle& value)
{
    _bindStyle->setStyle(value);
}

const LineStyle&
LineString::style() const
{
    return _bindStyle->style();
}

vsg::ref_ptr<vsg::Node>
LineString::getOrCreateNode(Runtime& runtime)
{
    // TODO: simple approach. Just put everything in every LineString for now.
    // We can optimize or group things later.

    auto stateGroup = vsg::StateGroup::create();
    stateGroup->stateCommands = LineState::pipelineStateCommands;
    //stateGroup->stateCommands.push_back(_bindStyle);
    stateGroup->addChild(_bindStyle);
    stateGroup->addChild(_geometry);

    return stateGroup;
}
