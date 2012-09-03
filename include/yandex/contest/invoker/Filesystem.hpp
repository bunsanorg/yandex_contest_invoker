#pragma once

#include "yandex/contest/invoker/filesystem/Config.hpp"

#include "yandex/contest/system/unistd/access/Id.hpp"

#include "yandex/contest/system/unistd/FileStatus.hpp"

#include <boost/noncopyable.hpp>
#include <boost/filesystem/path.hpp>

namespace yandex{namespace contest{namespace invoker
{
    /*!
     * \brief Object implements interface to the container's filesystem.
     */
    class Filesystem: private boost::noncopyable
    {
    public:
        Filesystem(const boost::filesystem::path &containerRoot,
                   const filesystem::Config &config);

        /*!
         * \brief Container's root directory.
         */
        const boost::filesystem::path &containerRoot() const;

        /*!
         * \brief Push local file into container.
         */
        void push(const boost::filesystem::path &local,
                  const boost::filesystem::path &remote,
                  const system::unistd::access::Id &ownerId,
                  const mode_t mode);

        /*!
         * \brief Push local file into container.
         */
        void pushLink(const boost::filesystem::path &local,
                      const boost::filesystem::path &remote,
                      const system::unistd::access::Id &ownerId,
                      const mode_t mode);

        system::unistd::FileStatus fileStatus(const boost::filesystem::path &remote);

        void setOwnerId(const boost::filesystem::path &remote,
                        const system::unistd::access::Id &ownerId);

        void setMode(const boost::filesystem::path &remote, const mode_t mode);

        /*!
         * \brief Pull remote directory tree.
         */
        void pull(const boost::filesystem::path &remote,
                  const boost::filesystem::path &local);

        ~Filesystem();

    private:
        const boost::filesystem::path containerRoot_;
    };
}}}
