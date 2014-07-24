#pragma once

#include <yandex/contest/invoker/filesystem/Config.hpp>

#include <yandex/contest/system/unistd/access/Id.hpp>
#include <yandex/contest/system/unistd/FileStatus.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

namespace yandex{namespace contest{namespace invoker
{
    struct FilesystemError: virtual Error
    {
        typedef boost::error_info<
            struct localPathTag,
            boost::filesystem::path
        > localPath;

        typedef boost::error_info<
            struct remotePathTag,
            boost::filesystem::path
        > remotePath;
    };

    struct FileExistsError: virtual FilesystemError {};
    struct FileDoesNotExistError: virtual FilesystemError {};

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

        /// Transform path in container to host path.
        boost::filesystem::path keepInRoot(
            const boost::filesystem::path &path) const;

        /// Transform host path into container path.
        boost::filesystem::path containerPath(
            const boost::filesystem::path &path) const;

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

        system::unistd::FileStatus fileStatus(
            const boost::filesystem::path &remote);

        void setOwnerId(const boost::filesystem::path &remote,
                        const system::unistd::access::Id &ownerId);

        void setMode(const boost::filesystem::path &remote,
                     const mode_t mode);

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
