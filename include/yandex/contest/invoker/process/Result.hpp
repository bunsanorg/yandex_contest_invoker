#pragma once

#include <yandex/contest/invoker/process/ResourceUsage.hpp>

#include <yandex/contest/system/unistd/ProcessResult.hpp>

#include <bunsan/stream_enum.hpp>

#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>

#include <utility>

namespace yandex{namespace contest{namespace invoker{namespace process
{
    struct Result: system::unistd::ProcessResult
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(exitStatus);
            ar & BOOST_SERIALIZATION_NVP(termSig);
            ar & BOOST_SERIALIZATION_NVP(completionStatus);
            ar & BOOST_SERIALIZATION_NVP(resourceUsage);
        }

        explicit Result(const int statLoc);

        Result()=default;
        Result(const Result &)=default;
        Result &operator=(const Result &)=default;

        explicit operator bool() const;

        BUNSAN_INCLASS_STREAM_ENUM_CLASS(CompletionStatus,
        (
            OK,
            ABNORMAL_EXIT,
            TERMINATED_BY_SYSTEM,
            MEMORY_LIMIT_EXCEEDED,
            USER_TIME_LIMIT_EXCEEDED,
            OUTPUT_LIMIT_EXCEEDED,
            START_FAILED,
            STOPPED
        ))

        CompletionStatus completionStatus = CompletionStatus::OK;

        ResourceUsage resourceUsage;
    };
}}}}
