#include <yandex/contest/invoker/lxc/Lxc.hpp>

#include <yandex/contest/system/execution/ErrCall.hpp>
#include <yandex/contest/system/unistd/Fstab.hpp>

#include <yandex/contest/StreamLog.hpp>

#include <lxc/lxccontainer.h>

#include <bunsan/filesystem/fstream.hpp>

#include <boost/filesystem/operations.hpp>

#include <chrono>
#include <thread>

namespace yandex{namespace contest{namespace invoker{namespace lxc
{
    using system::execution::ProcessArguments;
    namespace unistd = system::unistd;

    Lxc::Lxc(const std::string &name,
             const boost::filesystem::path &dir,
             const Config &config):
        name_(name),
        dir_(boost::filesystem::absolute(dir)),
        rootfs_(dir_ / "rootfs"),
        rootfsMount_(dir_ / "rootfs.mount"),
        configPath_(dir_ / "config"),
        container_(api::container_new(name_)),
        lastStart_(Clock::now())
    {
        STREAM_INFO << "Trying to create \"" << name_ << "\" LXC.";
        Config config_ = config;
        prepare(config_);
        STREAM_INFO << "Trying to create root directory "
                    << "for \"" << name_ << "\" LXC "
                    << "at " << rootfs_ << ".";
        boost::filesystem::create_directory(rootfs_);
        STREAM_INFO << "Root directory was successfully created "
                    << "for \"" << name_ << "\" LXC.";
        STREAM_INFO << "Trying to create root.mount directory "
                    << "for \"" << name_ << "\" LXC "
                    << "at " << rootfsMount_ << ".";
        boost::filesystem::create_directory(rootfsMount_);
        STREAM_INFO << "Root directory was successfully created "
                    << "for \"" << name_ << "\" LXC.";
        {
            STREAM_INFO << "Trying to write lxc.conf(5) "
                        << "for \"" << name_ << "\" LXC.";
            bunsan::filesystem::ofstream cfg(configPath_);
            BUNSAN_FILESYSTEM_FSTREAM_WRAP_BEGIN(cfg)
            {
                cfg << config_;
            }
            BUNSAN_FILESYSTEM_FSTREAM_WRAP_END(cfg)
            cfg.close();
            STREAM_INFO << "lxc.conf(5) was successfully written "
                        << "for \"" << name << "\" LXC.";
        }
        STREAM_DEBUG << "Loading LXC config for \"" << name_ << "\" at " << configPath_;
        if (!container_->load_config(container_.get(), configPath_.string().c_str()))
            BOOST_THROW_EXCEPTION(Error());
    }

    void Lxc::freeze()
    {
        STREAM_INFO << "Trying to freeze \"" << name_ << "\" LXC.";
        const system::execution::Result result =
            system::execution::getErrCallArgv("lxc-freeze", "-n", name_);
        if (result)
        {
            STREAM_INFO << "\"" << name_ << "\" LXC "
                        << "was successfully frozen.";
        }
        else
        {
            STREAM_ERROR << "Error while freezing \"" << name_
                         << "\" LXC: \"" << result.err << "\", "
                         << "exception is thrown.";
            BOOST_THROW_EXCEPTION(
                toUtilityError(result) <<
                Error::message("Error while freezing LXC."));
        }
    }

    void Lxc::unfreeze()
    {
        STREAM_INFO << "Trying to unfreeze \"" << name_ << "\" LXC.";
        const system::execution::Result result =
            system::execution::getErrCallArgv("lxc-unfreeze", "-n", name_);
        if (result)
        {
            STREAM_INFO << "\"" << name_ << "\" LXC "
                        << "was successfully unfrozen.";
        }
        else
        {
            STREAM_ERROR << "Error while unfreezing \"" << name_
                         << "\" LXC: \"" << result.err << "\", "
                         << "exception is thrown.";
            BOOST_THROW_EXCEPTION(
                toUtilityError(result) <<
                Error::message("Error while unfreezing LXC."));
        }
    }

    void Lxc::execute_(
        const Executor &executor,
        const system::execution::AsyncProcess::Options &options)
    {
        lastStart_ = Clock::now();
        // TODO thread-safety
        // TODO lxc-execute errors control
        STREAM_INFO << "Attempt to execute command "
                    << "in \"" << name_ << "\" LXC.";
        // we need to use it twice
        // and do not want it to change
        const State state_ = state();
        if (state_ != State::STOPPED)
        {
            STREAM_ERROR << "Command execution is impossible "
                         << "in \"" << name_ << "\" LXC "
                         << "due to illegal state, exception is thrown.";
            BOOST_THROW_EXCEPTION(
                IllegalStateError() <<
                IllegalStateError::state(state_) <<
                Error::message("It is impossible to spawn process in LXC."));
        }
        executor(transform(options));
        STREAM_INFO << "Command execution is started "
                    << "in \"" << name_ << "\" LXC.";
    }

    void Lxc::stop()
    {
        STREAM_INFO << "Trying to stop \"" << name_ << "\" LXC.";
        const State state_ = state();
        if (state_ == State::FROZEN)
        {
            STREAM_INFO << "\"" << name_
                        << "\" LXC is " << state_ << ", "
                        << "it should be unfrozen first.";
            unfreeze();
        }
        const auto end = lastStart_.load() + std::chrono::milliseconds(200);
        const auto step = std::chrono::milliseconds(25);
        do
        {
            const system::execution::Result result =
                system::execution::getErrCallArgv("lxc-stop", "-n", name_, "--kill");
            if (result)
            {
                STREAM_INFO << "\"" << name_ << "\" LXC "
                            << "was successfully stopped.";
                return;
            }
            else if (result.exitStatus && *result.exitStatus == 2)
            {
                STREAM_DEBUG << "\"" << name_
                             << "\" LXC is not running, nothing to be stopped. "
                             << "Sleeping to allow not-started LXC to start...";
                std::this_thread::sleep_for(step);
            }
            else
            {
                STREAM_ERROR << "Error while stopping \"" << name_
                             << "\" LXC: \"" << result.err << "\", "
                             << "exception is thrown.";
                BOOST_THROW_EXCEPTION(
                    toUtilityError(result) <<
                    Error::message("Error while stopping LXC."));
            }
        }
        while (Clock::now() < end);
        STREAM_INFO << "\"" << name_ << "\" LXC is not running.";
    }

    Lxc::State Lxc::state()
    {
        const char *const st = container_->state(container_.get());
        return boost::lexical_cast<State>(st);
    }

    Lxc::~Lxc()
    {
        STREAM_INFO << "Trying to remove \"" << name_ << "\" LXC.";
        try
        {
            stop();
        }
        catch (std::exception &)
        {
            STREAM_ERROR << "Unable to stop \""
                         << name_ << "\" LXC (ignoring).";
        }
        container_.reset(); // after stop && before remove
        boost::system::error_code ec;
        boost::filesystem::remove_all(dir_, ec);
        if (ec)
            STREAM_ERROR << "Unable to remove directory " << dir_ << " "
                         << "with \"" << name_ << "\" LXC "
                         << "due to \"" << ec << "\" (ignoring).";
        else
            STREAM_INFO << "\"" << name_ << "\" LXC "
                        << "was successfully removed.";
    }

    const boost::filesystem::path &Lxc::rootfs() const
    {
        return rootfs_;
    }

    void Lxc::prepare(Config &config)
    {
        config.rootfs = RootfsConfig{
            .fsname = rootfs_,
            .mount = rootfsMount_
        };
        if (config.mount)
        {
            if (config.mount->fstab)
            {
                unistd::Fstab fstab;
                fstab.load(config.mount->fstab.get());
                for (unistd::MountEntry &entry: fstab)
                    prepare(entry);
                const boost::filesystem::path fstab_ = dir_ / "fstab";
                fstab.save(fstab_);
                config.mount->fstab = fstab_;
            }
            if (config.mount->entries)
            {
                for (unistd::MountEntry &entry: config.mount->entries.get())
                    prepare(entry);
            }
        }
    }

    void Lxc::prepare(unistd::MountEntry &entry)
    {
        const boost::filesystem::path dst = rootfs_ / entry.dir;
        if (entry.hasOpt("bind") && !boost::filesystem::is_directory(entry.fsname))
        {
            boost::filesystem::create_directories(dst.parent_path());
            bunsan::filesystem::ofstream fout(dst);
            fout.close();
        }
        else
        {
            boost::filesystem::create_directories(dst);
        }
        entry.dir = dst.string();
    }

    system::execution::AsyncProcess::Options Lxc::transform(
        const system::execution::AsyncProcess::Options &options) const
    {
        system::execution::AsyncProcess::Options opts = options;
        opts.executable = "lxc-execute";
        opts.usePath = true;
        // options.arguments[1:]
        const ProcessArguments arguments(
            options.arguments.begin() + !options.arguments.empty(),
            options.arguments.end()
        );
        opts.arguments = system::execution::collect(
            "lxc-execute",
            "-n", name_,
            "-f", configPath_,
            "--", options.executable.string(), arguments
        );
        return opts;
    }

    UtilityError Lxc::toUtilityError(const system::execution::Result &result) const
    {
        return UtilityError(result) << Error::name(name_);
    }
}}}}
