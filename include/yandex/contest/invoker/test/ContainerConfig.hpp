#pragma once

#include <yandex/contest/invoker/ContainerConfig.hpp>
#include <yandex/contest/invoker/filesystem/Directory.hpp>

#include <bunsan/test/environment.hpp>

#include <algorithm>

namespace yandex{namespace contest{namespace invoker{namespace test
{
    static ContainerConfig getContainerConfig()
    {
        ContainerConfig config = ContainerConfig::fromEnvironment();
        config.controlProcessConfig.executable = bunsan::test::dir::binary() / "yandex_contest_invoker_ctl";
        // note: regular test setup is: src = dev/yandex.contest/invoker
        // bin = dev/yandex.contest/invoker/build
        const auto dev = bunsan::test::dir::source().parent_path().parent_path();
        // create all parent directories with 555 permissions
        const std::size_t dirs_begin_pos = config.filesystemConfig.createFiles.size();
        for (auto i = dev; i != i.root_path(); i = i.parent_path())
        {
            filesystem::Directory dir;
            dir.path = i;
            dir.mode = 0555;
            config.filesystemConfig.createFiles.emplace_back(dir);
        }
        std::reverse(config.filesystemConfig.createFiles.begin() + dirs_begin_pos,
                     config.filesystemConfig.createFiles.end());
        config.lxcConfig.mount->entries->push_back(
            system::unistd::MountEntry::bindRO(dev, dev)
        );
        return config;
    }
}}}}
