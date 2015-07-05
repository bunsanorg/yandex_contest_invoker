#include <yandex/contest/invoker/Container.hpp>

#include <yandex/contest/invoker/ProcessGroup.hpp>

#include <yandex/contest/system/execution/AsyncProcess.hpp>

#include <yandex/contest/detail/IntrusivePointerHelper.hpp>
#include <yandex/contest/SerializationCast.hpp>
#include <yandex/contest/StreamLog.hpp>
#include <yandex/contest/SystemError.hpp>

#include <boost/filesystem/operations.hpp>

namespace yandex{namespace contest{namespace invoker
{
    YANDEX_CONTEST_INTRUSIVE_PTR_DEFINE(Container)

    ContainerPointer Container::create(const ContainerConfig &config)
    {
        STREAM_INFO << "Trying to create new container";
        boost::filesystem::path path;
        /*
         * Here we try to create new directory
         * using random name. We will try for counter times.
         * boost::filesystem::create_directory() returns true
         * if new directory was created <=> we found unique name.
         */
        int counter = 10;
        bool newDirectoryCreated;
        do
        {
            path = boost::filesystem::unique_path(config.containersDir/"%%%%-%%%%-%%%%-%%%%");
        }
        while (--counter && !(newDirectoryCreated = boost::filesystem::create_directory(path)));
        if (!newDirectoryCreated)
        {
            STREAM_ERROR << "New container was not created due to name collisions, throwing an exception.";
            BOOST_THROW_EXCEPTION(Error() << Error::message("Unable to create unique container"));
        }
        // create Lxc
        std::unique_ptr<lxc::Lxc> lxcPtr;
        try
        {
            STREAM_INFO << "New container directory was created: " << path << ".";
            STREAM_INFO << "Trying to create LXC at " << path << " .";
            lxcPtr.reset(new lxc::Lxc(path.filename().string(), path, config.lxcConfig));
        }
        catch (...)
        {
            STREAM_ERROR << "Unable to create LXC at " << path << " .";
            boost::system::error_code ec;
            boost::filesystem::remove_all(path, ec);
            if (ec)
                STREAM_ERROR << "Unable to remove container's directory " << path <<
                                "due to \"" << ec << "\" (ignoring).";
            throw;
        }
        BOOST_ASSERT(lxcPtr);
        ContainerPointer container(new Container(std::move(lxcPtr), config));
        return container;
    }

    ContainerPointer Container::create(const ContainerConfig &config,
                                       const lxc::Config &lxcConfig)
    {
        ContainerConfig cfg = config;
        cfg.lxcConfig.patch(lxcConfig);
        return create(cfg);
    }

    ContainerPointer Container::create(const lxc::Config &lxcConfig)
    {
        return create(ContainerConfig(), lxcConfig);
    }

    ContainerPointer Container::create()
    {
        return create(ContainerConfig());
    }

    Container::Container(std::unique_ptr<lxc::Lxc> &&lxcPtr, const ContainerConfig &config):
        filesystem_(lxcPtr->rootfs(), config.filesystemConfig),
        controlProcessOptions_(config.controlProcessConfig),
        processGroupDefaultSettings_(config.processGroupDefaultSettings),
        lxcPtr_(std::move(lxcPtr))
    {
    }

    Filesystem &Container::filesystem()
    {
        return filesystem_;
    }

    ProcessGroupPointer Container::createProcessGroup()
    {
        return ProcessGroup::create(ContainerPointer(this));
    }

    const ProcessGroup::DefaultSettings &Container::processGroupDefaultSettings() const
    {
        return processGroupDefaultSettings_;
    }

    void Container::setProcessGroupDefaultSettings(
        const ProcessGroup::DefaultSettings &processGroupDefaultSettings)
    {
        processGroupDefaultSettings_ = processGroupDefaultSettings;
    }

    detail::execution::AsyncProcessGroup Container::execute(
        const detail::execution::AsyncProcessGroup::Task &task)
    {
        return lxcPtr_->execute(
            [&task](const system::execution::AsyncProcess::Options &options) ->
                detail::execution::AsyncProcessGroup
            {
                return detail::execution::AsyncProcessGroup(options, task);
            }, controlProcessOptions_);
    }

    void Container::stop()
    {
        lxcPtr_->stop();
    }
}}}
