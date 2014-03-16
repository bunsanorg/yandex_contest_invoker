#pragma once

#include <boost/serialization/access.hpp>
#include <bunsan/serialization/chrono.hpp>
#include <boost/serialization/nvp.hpp>

#include <chrono>

#include <cstdint>

namespace yandex{namespace contest{namespace invoker{namespace process
{
    struct ResourceLimits
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            using boost::serialization::make_nvp;

            ar & make_nvp("timeLimitNanos", timeLimit);
            ar & make_nvp("userTimeLimitMillis", userTimeLimit);
            ar & make_nvp("systemTimeLimitMillis", systemTimeLimit);
            ar & BOOST_SERIALIZATION_NVP(memoryLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(outputLimitBytes);
            ar & BOOST_SERIALIZATION_NVP(numberOfProcesses);
        }

        std::chrono::nanoseconds timeLimit = std::chrono::seconds(2);
        std::chrono::milliseconds userTimeLimit = std::chrono::hours(1);
        std::chrono::milliseconds systemTimeLimit = std::chrono::hours(1);
        std::uint64_t memoryLimitBytes = 256 * 1024 * 1024; // 256 MiB
        std::uint64_t outputLimitBytes = 2 * 1024 * 1024;   // 2 MiB

        /// Number of threads (RLIMIT_NPROC) for process real user id.
        std::uint64_t numberOfProcesses = 32;
    };
}}}}
