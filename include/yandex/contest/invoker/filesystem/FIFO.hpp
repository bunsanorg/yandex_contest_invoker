#pragma once

#include <yandex/contest/invoker/filesystem/File.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    struct FIFO: File
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & static_cast<File &>(*this);
        }

    protected:
        virtual void mknod() const;
    };
}}}}
