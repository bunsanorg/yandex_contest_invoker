#pragma once

#include "yandex/contest/invoker/process_group/ResourceUsage.hpp"

#include "yandex/contest/StreamEnum.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process_group
{
    namespace result_detail
    {
        YANDEX_CONTEST_STREAM_ENUM_CLASS(CompletionStatus,
        (
            OK,
            ABNORMAL_EXIT,
            REAL_TIME_LIMIT_EXCEEDED,
            STOPPED
        ))
    }

    struct Result
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(completionStatus);
            ar & BOOST_SERIALIZATION_NVP(resourceUsage);
        }

        typedef result_detail::CompletionStatus CompletionStatus;

        CompletionStatus completionStatus;
        ResourceUsage resourceUsage;
    };
}}}}
