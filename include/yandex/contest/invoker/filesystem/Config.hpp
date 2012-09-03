#pragma once

#include "yandex/contest/invoker/filesystem/CreateFile.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    struct Config
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(createFiles);
        }

        CreateFiles createFiles;
    };
}}}}
