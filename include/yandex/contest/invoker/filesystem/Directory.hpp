#pragma once

#include "yandex/contest/invoker/filesystem/File.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    /*!
     * \brief Directory configuration.
     *
     * If directory exists owner and permissions
     * will be updated.
     *
     * \note We do not need any fields here
     * because the only thing
     * can be configured is contents,
     * but it is configured elsewhere.
     */
    struct Directory: File
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
