#pragma once

#include "Notifier.hpp"
#include "ProcessInfo.hpp"

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
        explicit ExecutionMonitor(
            const std::vector<AsyncProcessGroup::Process> &processes):
                resourceLimits_(processes.size())
        {
            for (std::size_t i = 0; i < processes.size(); ++i)
                resourceLimits_[i] = processes[i].resourceLimits;
            result_.processGroupResult.completionStatus =
                process_group::Result::CompletionStatus::OK;
            result_.processResults.resize(processes.size());
        }

        /// Notify monitor that process has started.
        void started(
            ProcessInfo &processInfo,
            const AsyncProcessGroup::Process &process);

        /// Notify monitor that process has terminated.
        void terminated(ProcessInfo &processInfo, const int statLoc);

        void terminatedBySystem(ProcessInfo &processInfo);

        /// All processes has terminated.
        void allTerminated();

        /// Notify monitor that real time limit was exceeded.
        void realTimeLimitExceeded();

        bool runOutOfResourceLimits(ProcessInfo &processInfo);

        process::Result::CompletionStatus collectResourceInfo(
            ProcessInfo &processInfo);

        bool processGroupIsRunning() const
        {
            return result_.processGroupResult.completionStatus ==
                       process_group::Result::CompletionStatus::OK &&
                   !groupWaitsForTermination_.empty();
        }

        bool processesAreRunning() const { return !running_.empty(); }

        const AsyncProcessGroup::Result &result() const;

        const std::unordered_set<Id> &running() const { return running_; }

        boost::signals2::connection onSpawn(
            const Notifier::SpawnSignal::slot_type &slot)
        {
            return signals_.spawn.connect(slot);
        }

        boost::signals2::connection onTermination(
            const Notifier::TerminationSignal::slot_type &slot)
        {
            return signals_.termination.connect(slot);
        }

        boost::signals2::connection onClose(
            const Notifier::CloseSignal::slot_type &slot)
        {
            return signals_.close.connect(slot);
        }

    private:
        Notifier::Signals signals_;
        std::vector<process::ResourceLimits> resourceLimits_;
        AsyncProcessGroup::Result result_;
        std::unordered_set<Id> running_, terminated_,
            terminateGroupOnCrash_, groupWaitsForTermination_;
    };
}}}}}}
