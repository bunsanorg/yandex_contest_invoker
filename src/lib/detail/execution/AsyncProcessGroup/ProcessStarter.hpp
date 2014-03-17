#pragma once

#include "ProcessInfo.hpp"

#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>

#include <yandex/contest/system/cgroup/ControlGroup.hpp>
#include <yandex/contest/system/unistd/Descriptor.hpp>
#include <yandex/contest/system/unistd/Exec.hpp>
#include <yandex/contest/system/unistd/Pipe.hpp>

#include <boost/noncopyable.hpp>

#include <unordered_map>
#include <unordered_set>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    class ProcessStarter: private boost::noncopyable
    {
    public:
        ProcessStarter(system::cgroup::ControlGroup &controlGroup,
                       const AsyncProcessGroup::Process &process,
                       std::vector<system::unistd::Pipe> &pipes);

        /// Start process and return it's pid.
        Pid operator()();

    private:
        /// Never returns.
        void startChild() noexcept;

        void childCloseFds();

        void childSetUpFds();

        void setUpControlGroup();

        void childSetUpResourceLimits();

        /// For resource limits that depends on user id.
        void childSetUpResourceLimitsUser();

    private:
        system::cgroup::ControlGroup &controlGroup_;
        system::unistd::access::Id ownerId_;
        system::unistd::Exec exec_;
        std::unordered_map<int, int> descriptors_;
        std::vector<system::unistd::Descriptor> allocatedFds_;
        std::unordered_set<int> childCloseFds_;
        boost::filesystem::path currentPath_;
        process::ResourceLimits resourceLimits_;
    };
}}}}}}
