#pragma once

#include "yandex/contest/system/unistd/access/Id.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/serialization/variant.hpp>
#include "yandex/contest/serialization/path.hpp"

#include <sys/types.h>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    struct File
    {
        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(path);
            ar & BOOST_SERIALIZATION_NVP(mode);
            ar & BOOST_SERIALIZATION_NVP(ownerId);
        }

        /*!
         * \brief Create file.
         *
         * If parent path does not exists,
         * all directories will be created
         * (uid=gid=0, mode=0777).
         */
        void create() const;

        boost::filesystem::path path;

        mode_t mode = 0777;

        system::unistd::access::Id ownerId;

    protected:
        /*!
         * \brief Appropriate call to mknod(3) or something similar.
         *
         * \note Should be implemented in derived class
         * with needed configuration settings.
         */
        virtual void mknod() const=0;

        /*!
         * \brief Calls chmod(3).
         *
         * It may be used by some derived classes.
         */
        void chmod() const;

    private:
        void chown() const;
    };
}}}}
