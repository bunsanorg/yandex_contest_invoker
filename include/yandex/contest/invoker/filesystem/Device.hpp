#pragma once

#include "yandex/contest/invoker/filesystem/File.hpp"

#include "bunsan/stream_enum.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/export.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    /// See mknod(3), makedev(3).
    struct Device: File
    {
        BUNSAN_INCLASS_STREAM_ENUM(Type,
        (
            CHAR,
            BLOCK
        ))

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & static_cast<File &>(*this);
            ar & BOOST_SERIALIZATION_NVP(type);
            ar & BOOST_SERIALIZATION_NVP(major);
            ar & BOOST_SERIALIZATION_NVP(minor);
        }

        Type type;
        int major;
        int minor;

    protected:
        virtual void mknod() const;
    };
}}}}
