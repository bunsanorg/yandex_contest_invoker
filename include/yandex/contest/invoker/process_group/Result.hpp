#pragma once

#include "yandex/contest/invoker/process_group/ResourceUsage.hpp"

#include "bunsan/stream_enum.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process_group
{
    struct Result
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(completionStatus);
            ar & BOOST_SERIALIZATION_NVP(resourceUsage);
        }

        BUNSAN_INCLASS_STREAM_ENUM_CLASS(CompletionStatus,
        (
            OK,
            ABNORMAL_EXIT,
            REAL_TIME_LIMIT_EXCEEDED,
            STOPPED
        ))

        CompletionStatus completionStatus;
        ResourceUsage resourceUsage;
    };
}}}}
