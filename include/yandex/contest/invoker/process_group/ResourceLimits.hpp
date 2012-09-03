#pragma once

#include <cstdint>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process_group
{
    struct ResourceLimits
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(realTimeLimitMillis);
        }

        std::uint64_t realTimeLimitMillis = 10 * 1000; // 10s
    };
}}}}
