#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

#include <string>

#include <cstdint>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    struct ProcessId
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(id);
            ar & BOOST_SERIALIZATION_NVP(name);
        }

        std::size_t id = 0;
        std::string name;
    };
}}}}
