#pragma once

#include <cstdint>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process
{
    struct ResourceLimits
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(timeLimitMillis);
            ar & BOOST_SERIALIZATION_NVP(memoryLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(hardMemoryLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(outputLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(numberOfProcesses);
        }

        std::uint64_t timeLimitMillis = 2 * 1000;               // 2 s
        std::uint64_t memoryLimitBytes = 256 * 1024 * 1024;     // 256 MiB
        std::uint64_t hardMemoryLimitBytes = 512 * 1024 * 1024; // 512 MiB
        std::uint64_t outputLimitBytes = 2 * 1024 * 1024;       // 2 MiB

        /// Number of threads (RLIMIT_NPROC) for process real user id.
        std::uint64_t numberOfProcesses = 32;
    };
}}}}
