#include <yandex/contest/invoker/lxc/Config.hpp>

#include <yandex/contest/invoker/lxc/ConfigHelper.hpp>

namespace yandex{namespace contest{namespace invoker{namespace lxc
{
    void Config::patch(const Config &config)
    {
        BOOST_OPTIONAL_OVERRIDE_PATCH(arch);
        BOOST_OPTIONAL_OVERRIDE_PATCH(utsname);
        // TODO network
        // TODO pts
        BOOST_OPTIONAL_OVERRIDE_PATCH(console);
        BOOST_OPTIONAL_OVERRIDE_PATCH(tty);
        BOOST_OPTIONAL_OVERRIDE_PATCH(devttydir);
        BOOST_OPTIONAL_RECURSIVE_PATCH(mount);
        BOOST_OPTIONAL_RECURSIVE_PATCH(rootfs);
        BOOST_OPTIONAL_OVERRIDE_PATCH(pivotdir);
        BOOST_OPTIONAL_OVERRIDE_PATCH(cgroup);
        BOOST_OPTIONAL_OVERRIDE_PATCH(cap_drop);
    }

    std::ostream &operator<<(std::ostream &out, const Config &config)
    {
        using namespace config_helper;
        optionalOutput(out, "lxc.arch", config.arch);
        optionalOutput(out, "lxc.utsname", config.utsname);
        // TODO network
        // TODO pts
        optionalOutput(out, "lxc.console", config.console);
        optionalOutput(out, "lxc.tty", config.tty);
        optionalOutput(out, "lxc.devttydir", config.devttydir);
        if (config.mount)
            out << config.mount.get();
        if (config.rootfs)
            out << config.rootfs.get();
        optionalOutput(out, "lxc.pivotdir", config.pivotdir);
        if (config.cgroup)
            for (const auto &cgrp: config.cgroup.get())
                output(out, "lxc.cgroup." + cgrp.first, cgrp.second);
        if (config.cap_drop)
            for (const auto &capDrop: config.cap_drop.get())
                output(out, "lxc.cap.drop", capDrop);
        return out;
    }
}}}}
