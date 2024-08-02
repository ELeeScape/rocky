/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/Common.h>
#include <vsg/core/Version.h>

#define ROCKY_VSG_NAMESPACE ROCKY_NAMESPACE

// non-exported classes
#define ROCKY_VSG_INTERNAL

// introduced in VSG 1.1.4
#ifndef VSG_API_VERSION_LESS
#define VSG_API_VERSION_LESS(major, minor, patch) ((VSG_VERSION_MAJOR < major) || ((VSG_VERSION_MAJOR == major) && ((VSG_VERSION_MINOR < minor) || ((VSG_VERSION_MINOR==minor) && VSG_VERSION_PATCH < patch))))
#define VSG_API_VERSION_GREATER_EQUAL(major, minor, patch) ((VSG_VERSION_MAJOR > major) || ((VSG_VERSION_MAJOR == major) && ((VSG_VERSION_MINOR > minor) || ((VSG_VERSION_MINOR==minor) && VSG_VERSION_PATCH >= patch))))
#endif

#if 0
#include <chrono>

namespace ROCKY_NAMESPACE
{
    using clock = std::chrono::steady_clock;
}

#define ROCKY_VSG_NAMESPACE ROCKY_NAMESPACE

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
    #  if defined( ROCKY_VSG_LIBRARY_STATIC )
    #    define ROCKY_EXPORT
    #  elif defined( ROCKY_VSG_LIBRARY )
    #    define ROCKY_EXPORT   __declspec(dllexport)
    #  else
    #    define ROCKY_EXPORT   __declspec(dllimport)
    #  endif
#else
    #  define ROCKY_EXPORT
#endif


// marker for non-exported classes
#define ROCKY_VSG_INTERNAL
#endif