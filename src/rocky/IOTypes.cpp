/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "IOTypes.h"
#include "Instance.h"
#include "json.h"

using namespace ROCKY_NAMESPACE;

const std::string IOMetadata::CONTENT_TYPE = "Content-Type";


namespace ROCKY_NAMESPACE
{
    void to_json(json& j, const ProxySettings& obj) {
        j = json::object();
        set(j, "host", obj.hostname);
        set(j, "port", obj.port);
        set(j, "username", obj.username);
        set(j, "password", obj.password);
    }

    void from_json(const json& j, ProxySettings& obj) {
        get_to(j, "host", obj.hostname);
        get_to(j, "port", obj.port);
        get_to(j, "username", obj.username);
        get_to(j, "password", obj.password);
    }
}


//------------------------------------------------------------------------

CachePolicy::CachePolicy()
{
    //nop
}
CachePolicy::CachePolicy(const Usage& u) :
    usage(u)
{
    //nop
}

void
CachePolicy::mergeAndOverride(const CachePolicy& rhs)
{
    if (rhs.usage.has_value())
        usage = rhs.usage.value();

    if (rhs.minTime.has_value())
        minTime = rhs.minTime.value();

    if (rhs.maxAge.has_value())
        maxAge = rhs.maxAge.value();
}

void
CachePolicy::mergeAndOverride(const optional<CachePolicy>& rhs)
{
    if (rhs.has_value())
    {
        mergeAndOverride(rhs.value());
    }
}

DateTime
CachePolicy::getMinAcceptTime() const
{
    return
        minTime.has_value() ? minTime.value() :
        maxAge.has_value() ? DateTime().asTimeStamp() - (TimeStamp)maxAge.value() :
        0;
}

bool
CachePolicy::isExpired(TimeStamp lastModified) const
{
    return lastModified < getMinAcceptTime().asTimeStamp();
}

bool
CachePolicy::operator == (const CachePolicy& rhs) const
{
    return
        (usage.value() == rhs.usage.value()) &&
        (maxAge.value() == rhs.maxAge.value()) &&
        (minTime.value() == rhs.minTime.value());
}

CachePolicy&
CachePolicy::operator = (const CachePolicy& rhs)
{
    usage = optional<Usage>(rhs.usage);
    maxAge = optional<Duration>(rhs.maxAge);
    minTime = optional<DateTime>(rhs.minTime);

    return *this;
}

std::string
CachePolicy::usageString() const
{
    if (usage == Usage::READ_WRITE) return "read-write";
    if (usage == Usage::READ_ONLY)  return "read-only";
    if (usage == Usage::CACHE_ONLY)  return "cache-only";
    if (usage == Usage::NO_CACHE)    return "no-cache";
    return "unknown";
}

IOOptions::IOOptions(const IOOptions& rhs)
{
    this->operator=(rhs);
}

IOOptions::IOOptions(const IOOptions& rhs, Cancelable& c) :
    IOOptions(rhs)
{
    _cancelable = &c;
}

IOOptions::IOOptions(Cancelable& c) :
    _cancelable(&c)
{
    //nop
}

IOOptions::IOOptions(const std::string& in_referrer) :
    referrer(in_referrer)
{
    //nop
}

IOOptions&
IOOptions::operator = (const IOOptions& rhs)
{
    services = rhs.services;
    referrer = rhs.referrer;
    maxNetworkAttempts = rhs.maxNetworkAttempts;
    _cancelable = rhs._cancelable;
    _properties = rhs._properties;
    return *this;
}

namespace
{
    ReadImageURIService default_read_image_from_uri = [](const std::string&, const IOOptions&) { return Status(Status::ServiceUnavailable); };
    ReadImageStreamService default_read_image_from_stream = [](std::istream&, const std::string&, const IOOptions&) { return Status(Status::ServiceUnavailable); };
    CacheService default_cache = []() { return nullptr; };
}

Services::Services() :
    readImageFromURI(default_read_image_from_uri),
    readImageFromStream(default_read_image_from_stream),
    cache(default_cache)
{
    //nop
}

