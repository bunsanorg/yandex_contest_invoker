#pragma once

#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>

#include <yandex/contest/system/cgroup/ControlGroup.hpp>

#include <boost/noncopyable.hpp>

#include <unordered_set>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    typedef std::size_t Id;

    class ExecutionMonitor: private boost::noncopyable
    {
    public:
        explicit ExecutionMonitor(const std::vector<AsyncProcessGroup::Process> &processes):
            resourceLimits_(processes.size())
        {
            for (std::size_t i = 0; i < processes.size(); ++i)
                resourceLimits_[i] = processes[i].resourceLimits;
            result_.processGroupResult.completionStatus = process_group::Result::CompletionStatus::OK;
            result_.processResults.resize(processes.size());
        }

        /// Notify monitor that process with specified id has started.
        void started(const Id id, const AsyncProcessGroup::Process &process);

        /// Notify monitor that process with specified id has terminated.
        void terminated(const Id id, const int statLoc,
                        system::cgroup::ControlGroup &controlGroup);

        void terminatedBySystem(const Id id);

        /// Notify monitor that real time limit was exceeded.
        void realTimeLimitExceeded();

        bool runOutOfResourceLimits(const Id id, system::cgroup::ControlGroup &controlGroup);

        process::Result::CompletionStatus collectResourceInfo(
            const Id id, system::cgroup::ControlGroup &controlGroup);

        bool processGroupIsRunning() const
        {
            return result_.processGroupResult.completionStatus == process_group::Result::CompletionStatus::OK &&
                   !groupWaitsForTermination_.empty();
        }

        bool processesAreRunning() const { return !running_.empty(); }

        const AsyncProcessGroup::Result &result() const;

        const std::unordered_set<Id> &running() const { return running_; }

    private:
        std::vector<process::ResourceLimits> resourceLimits_;
        AsyncProcessGroup::Result result_;
        std::unordered_set<Id> running_, terminated_,
            terminateGroupOnCrash_, groupWaitsForTermination_;
    };
}}}}}}
