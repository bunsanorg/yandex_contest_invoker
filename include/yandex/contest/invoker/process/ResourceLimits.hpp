#pragma once

#include <chrono>

#include <cstdint>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include "yandex/contest/serialization/chrono.hpp"

namespace yandex{namespace contest{namespace invoker{namespace process
{
    struct ResourceLimits
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & boost::serialization::make_nvp("userTimeLimitMillis", userTimeLimit);
            ar & BOOST_SERIALIZATION_NVP(memoryLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(outputLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(numberOfProcesses);
        }

        std::chrono::milliseconds userTimeLimit = std::chrono::seconds(2);
        std::uint64_t memoryLimitBytes = 256 * 1024 * 1024; // 256 MiB
        std::uint64_t outputLimitBytes = 2 * 1024 * 1024;   // 2 MiB

        /// Number of threads (RLIMIT_NPROC) for process real user id.
        std::uint64_t numberOfProcesses = 32;
    };
}}}}
