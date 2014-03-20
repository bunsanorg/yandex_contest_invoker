#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/system/error_code.hpp>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    /// Local error
    struct ErrorEvent
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(errorCode);
        }

        boost::system::error_code errorCode;
    };
}}}}
