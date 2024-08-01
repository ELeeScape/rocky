/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/Common.h>
#include <rocky/GeoCommon.h>
#include <rocky/GeoExtent.h>

namespace ROCKY_NAMESPACE
{
    class Image;
    class GeoExtent;

    /**
     * A georeferenced image; i.e. an Image coupled with a GeoExtent.
     */
    class ROCKY_EXPORT GeoImage
    {
    public:

        //! Construct an empty (invalid) geoimage
        GeoImage();
        GeoImage(const GeoImage&) = default;
        GeoImage& operator=(const GeoImage&) = default;
        GeoImage(const GeoImage&& rhs) { *this = rhs; }
        GeoImage& operator=(GeoImage&&);

        //! Constructs a new goereferenced image.
        GeoImage(shared_ptr<Image> image, const GeoExtent& extent);

        //! Destructor
        virtual ~GeoImage() { }

        static GeoImage INVALID;

        //! True if this is a valid geo image.
        bool valid() const;

        //! Gets a pointer to the underlying image
        shared_ptr<Image> image() const;

        //! Gets the geospatial extent of the image
        const GeoExtent& extent() const;

        //! Shortcut to get the spatial reference system describing
        //! the projection of the image.
        const SRS& srs() const;

        /**
         * Crops the image to a new geospatial extent.
         *
         * @param extent
         *      New extent to which to crop the image.
         * @param exact
         *      If "exact" is true, the output image will have exactly the extents requested;
         *      this process may require resampling and will therefore be more expensive. If
         *      "exact" is false, we do a simple crop of the image that is rounded to the nearest
         *      pixel. The resulting extent will be close but usually not exactly what was
         *      requested - however, this method is faster.
         * @param width, height
         *      New pixel size for the output image. By default, the method will automatically
         *      calculate a new pixel size.
         */
        Result<GeoImage> crop(
            const GeoExtent& extent,
            bool exact = false,
            unsigned int width = 0,
            unsigned int height = 0,
            bool useBilinearInterpolation = true) const;

        //! Warps the image into a new spatial reference system.
        //!
        //! @param to_srs SRS into which to warp the image.
        //! @param to_extent Supply this extent if you wish to warp AND crop the image
        //!   in one step. This is faster than calling reproject() and then crop().
        //! @param width, height New pixel size for the output image. Be default,
        //!   the method will automatically calculate a new pixel size.
        Result<GeoImage> reproject(
            const SRS& to_srs,
            const GeoExtent* to_extent = nullptr,
            unsigned width = 0,
            unsigned height = 0,
            bool useBilinearInterpolation = true) const;

        //! Composites one or more source images into this image.
        //! This will only write to RGBA pixels that have an alpha of zero,
        //! or to RGB pixels that all all black.
        //! @param sources GeoImages to composite, from bottom to top.
        void composite(const std::vector<GeoImage>& sources);

        //! Gets the units per pixel of this geoimage
        double getUnitsPerPixel() const;

        //! Gets the coordinate at the image's s,t
        bool getCoord(int s, int t, double& out_x, double& out_y) const;

        //! Read the value of a pixel at a geopoint.
        bool read(glm::fvec4& output, const GeoPoint& p) const;

        //! Read the value of a pixel at the coordinate (x, y) which is
        //! assumed to be in this GeoImage's SRS.
        bool read(
            glm::fvec4& output,
            double x, double y) const;

        //! Clamp the input coordinate to the image's valid extent and then
        //! read the value of a pixel at the coordinate (x, y) which is
        //! assumed to be in the GeoImage's SRS.
        bool read_clamped(
            glm::fvec4& output,
            double x, double y) const;

        //! Read the value of a pixel at the coordinate (x, y) which
        //! is expressed in the provided SRS.
        bool read(
            glm::fvec4& output,
            double x, double y, const SRS& srs) const;

        //! Read the value of a pixel at the coordinate (x, y) which
        //! will be transformed using the provided operation before
        //! reading.
        bool read(
            glm::fvec4& output,
            double x, double y, const SRSOperation& operation) const;

    private:
        GeoExtent _extent;
        shared_ptr<Image> _image;
    };
}
