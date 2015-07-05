#pragma once

#include <yandex/contest/invoker/ContainerConfig.hpp>

#include <bunsan/test/environment.hpp>

namespace yandex{namespace contest{namespace invoker{namespace tests
{
    static ContainerConfig getContainerConfig()
    {
        ContainerConfig config = ContainerConfig::fromEnvironment();
        config.controlProcessConfig.executable = bunsan::test::dir::binary() / "yandex_contest_invoker_ctl";
        config.lxcConfig.mount->entries->push_back(system::unistd::MountEntry::bindRO(
            bunsan::test::dir::binary(),
            bunsan::test::dir::binary()
        ));
        return config;
    }
}}}}
