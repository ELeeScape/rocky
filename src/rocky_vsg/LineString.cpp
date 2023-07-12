/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "LineString.h"
#include "json.h"
#include <vsg/nodes/DepthSorted.h>

using namespace ROCKY_NAMESPACE;

LineString::LineString() :
    super()
{
    _geometry = LineStringGeometry::create();
    _bindStyle = BindLineStyle::create();
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

void
LineString::createNode(Runtime& runtime)
{
    // TODO: simple approach. Just put everything in every LineString for now.
    // We can optimize or group things later.
    if (!node)
    {
        ROCKY_HARD_ASSERT(LineState::status.ok());
        auto stateGroup = vsg::StateGroup::create();
        stateGroup->stateCommands = LineState::pipelineStateCommands;
        stateGroup->addChild(_bindStyle);
        stateGroup->addChild(_geometry);
        auto sw = vsg::Switch::create();
        sw->addChild(true, stateGroup);
        node = sw;
    }
}

JSON
LineString::to_json() const
{
    ROCKY_SOFT_ASSERT(false, "Not yet implemented");
    auto j = json::object();
    set(j, "name", name);
    return j.dump();
}



MultiLineString::MultiLineString() :
    super()
{
    _bindStyle = BindLineStyle::create();
}

void
MultiLineString::setStyle(const LineStyle& value)
{
    _bindStyle->setStyle(value);
}

const LineStyle&
MultiLineString::style() const
{
    return _bindStyle->style();
}

void
MultiLineString::createNode(Runtime& runtime)
{
    // TODO: simple approach. Just put everything in every LineString for now.
    // We can optimize or group things later.
    if (!node)
    {
        ROCKY_HARD_ASSERT(LineState::status.ok());

        auto stateGroup = vsg::StateGroup::create();
        stateGroup->stateCommands = LineState::pipelineStateCommands;
        
        stateGroup->addChild(_bindStyle);

        for (auto& geom : _geometries)
            stateGroup->addChild(geom);

        auto sw = vsg::Switch::create();
        sw->addChild(true, stateGroup);
        node = sw;
    }
}

JSON
MultiLineString::to_json() const
{
    ROCKY_SOFT_ASSERT(false, "Not yet implemented");
    auto j = json::object();
    set(j, "name", name);
    return j.dump();
}