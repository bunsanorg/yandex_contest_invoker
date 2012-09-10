#pragma once

#include "yandex/contest/invoker/filesystem/File.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/filesystem/path.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    struct SymLink: File
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & static_cast<File &>(*this);
            ar & BOOST_SERIALIZATION_NVP(value);
        }

        boost::filesystem::path value;

    protected:
        virtual void mknod() const;
    };
}}}}
