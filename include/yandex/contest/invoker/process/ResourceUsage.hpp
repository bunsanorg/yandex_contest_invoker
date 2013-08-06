#pragma once

#include <boost/serialization/access.hpp>
#include <bunsan/serialization/chrono.hpp>
#include <boost/serialization/nvp.hpp>

#include <chrono>

#include <cstdint>

namespace yandex{namespace contest{namespace invoker{namespace process
{
    struct ResourceUsage
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & boost::serialization::make_nvp("timeUsageNanos", timeUsage);
            ar & boost::serialization::make_nvp("userTimeUsageMillis", userTimeUsage);
            ar & boost::serialization::make_nvp("systemTimeUsageMillis", systemTimeUsage);
            ar & BOOST_SERIALIZATION_NVP(memoryUsageBytes);
        }

        ResourceUsage()=default;
        ResourceUsage(const ResourceUsage &)=default;
        ResourceUsage &operator=(const ResourceUsage &)=default;

        std::chrono::nanoseconds timeUsage;
        std::chrono::milliseconds userTimeUsage;
        std::chrono::milliseconds systemTimeUsage;
        std::uint64_t memoryUsageBytes = 0;
    };
}}}}
