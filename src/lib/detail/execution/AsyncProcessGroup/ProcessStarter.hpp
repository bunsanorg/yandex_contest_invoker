#pragma once

#include <yandex/contest/system/unistd/Exec.hpp>
#include <yandex/contest/system/unistd/Pipe.hpp>
#include <yandex/contest/system/unistd/Descriptor.hpp>

#include <yandex/contest/system/cgroup/ControlGroup.hpp>

#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>

#include <unordered_map>
#include <unordered_set>

#include <boost/noncopyable.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    typedef ::pid_t Pid;

    /*!
     * \brief Signal indicating that child start has failed.
     *
     * If child process unable to exec(3)
     * it raises SIG_START_FAILED. Signal value
     * is implementation-defined.
     *
     * \warning When ProcessResult::completionStatus ==
     * ProcessResult::CompletionStatus::START_FAILED other fields
     * have unspecified values (e.g. user should not assume that termSig is set).
     */
    extern const int SIG_START_FAILED;

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

        void childCloseFDs();

        void childSetUpFDs();

        void setUpControlGroup();

        void childSetUpResourceLimits();

        /// For resource limits that depends on user id.
        void childSetUpResourceLimitsUser();

    private:
        system::cgroup::ControlGroup &controlGroup_;
        system::unistd::access::Id ownerId_;
        system::unistd::Exec exec_;
        std::unordered_map<int, int> descriptors_;
        std::vector<system::unistd::Descriptor> allocatedFDs_;
        std::unordered_set<int> childCloseFDs_;
        boost::filesystem::path currentPath_;
        process::ResourceLimits resourceLimits_;
    };
}}}}}}
