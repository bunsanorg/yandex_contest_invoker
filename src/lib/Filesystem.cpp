// warning: need to be the first include
#include <boost/config.hpp>
#define BOOST_NO_SCOPED_ENUMS

#include "yandex/contest/invoker/Filesystem.hpp"

#include "yandex/contest/invoker/filesystem/Operations.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include "yandex/contest/detail/LogHelper.hpp"

#include <boost/filesystem/operations.hpp>

namespace yandex{namespace contest{namespace invoker
{
    Filesystem::Filesystem(const boost::filesystem::path &containerRoot,
                           const filesystem::Config &config):
        containerRoot_(boost::filesystem::absolute(containerRoot))
    {
        STREAM_INFO << "Trying to create requested files in container.";
        for (filesystem::CreateFile createFile: config.createFiles)
            createFile.create(containerRoot_);
        STREAM_INFO << "Requested files were successfully created.";
    }

    Filesystem::~Filesystem()
    {
    }

    const boost::filesystem::path &Filesystem::containerRoot() const
    {
        return containerRoot_;
    }

    void Filesystem::push(const boost::filesystem::path &local,
                          const boost::filesystem::path &remote,
                          const system::unistd::access::Id &ownerId,
                          const mode_t mode)
    {
        const boost::filesystem::path remote_ = filesystem::keepInRoot(remote, containerRoot_);
        STREAM_DEBUG << "Attempt to push " << local << " to " << remote << ".";
        boost::filesystem::create_directories(remote_.parent_path());
        boost::filesystem::copy_file(local, remote_,
                                     boost::filesystem::copy_option::overwrite_if_exists);
        setOwnerId(remote, ownerId);
        setMode(remote, mode);
    }

    void Filesystem::pushLink(const boost::filesystem::path &local,
                              const boost::filesystem::path &remote,
                              const system::unistd::access::Id &ownerId,
                              const mode_t mode)
    {
        const boost::filesystem::path remote_ = filesystem::keepInRoot(remote, containerRoot_);
        STREAM_DEBUG << "Attempt to push hard link " << local <<
            " to " << remote << "(" << remote_ << ")" << ".";
        boost::filesystem::create_directories(remote_.parent_path());
        boost::filesystem::create_hard_link(local, remote_);
        setOwnerId(remote, ownerId);
        setMode(remote, mode);
    }

    system::unistd::FileStatus Filesystem::fileStatus(const boost::filesystem::path &remote)
    {
        const boost::filesystem::path remote_ = filesystem::keepInRoot(remote, containerRoot_);
        STREAM_DEBUG << "Attempt to get file status " << remote << "(" << remote_ << ")" << ".";
        return system::unistd::stat(remote_);
    }

    void Filesystem::setOwnerId(const boost::filesystem::path &remote,
                                const system::unistd::access::Id &ownerId)
    {
        const boost::filesystem::path remote_ = filesystem::keepInRoot(remote, containerRoot_);
        STREAM_DEBUG << "Attempt to chown " << remote << "(" << remote_ << ")" << ".";
        system::unistd::chown(remote_, ownerId);
    }

    void Filesystem::setMode(const boost::filesystem::path &remote, const mode_t mode)
    {
        const boost::filesystem::path remote_ = filesystem::keepInRoot(remote, containerRoot_);
        STREAM_DEBUG << "Attempt to chmod " << remote << "(" << remote_ << ")" << ".";
        system::unistd::chmod(remote_, mode);
    }

    void Filesystem::pull(const boost::filesystem::path &remote,
                          const boost::filesystem::path &local)
    {
        const boost::filesystem::path remote_ = filesystem::keepInRoot(remote, containerRoot_);
        STREAM_DEBUG << "Attempt to pull " << remote << "(" << remote_ << ")" <<
            " to " << local << ".";
        boost::filesystem::create_directories(local.parent_path());
        boost::filesystem::copy(remote_, local);
    }
}}}
