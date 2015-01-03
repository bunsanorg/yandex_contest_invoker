#include "ExecutionMonitor.hpp"

#include <yandex/contest/detail/LogHelper.hpp>

#include <boost/assert.hpp>

#include <signal.h>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    void ExecutionMonitor::started(
        ProcessInfo &processInfo,
        const AsyncProcessGroup::Process &process)
    {
        const std::size_t id = processInfo.id();

        STREAM_TRACE << "Started " << processInfo << ".";
        BOOST_ASSERT_MSG(running_.find(id) == running_.end(),
                         "This process has already started.");
        running_.insert(id);
        if (process.groupWaitsForTermination)
            groupWaitsForTermination_.insert(id);
        if (process.terminateGroupOnCrash)
            terminateGroupOnCrash_.insert(id);

        signals_.spawn(processInfo.meta());
    }

    void ExecutionMonitor::terminated(
        ProcessInfo &processInfo, const int statLoc)
    {
        const std::size_t id = processInfo.id();

        STREAM_TRACE << "Terminated " << processInfo << ".";
        BOOST_ASSERT(running_.size() + terminated_.size() ==
                     result_.processResults.size());
        BOOST_ASSERT_MSG(running_.find(id) != running_.end(),
                         "This process has already terminated.");
        BOOST_ASSERT_MSG(terminated_.find(id) == terminated_.end(),
                         "This process has already terminated.");
        // move id
        running_.erase(id);
        terminated_.insert(id);
        groupWaitsForTermination_.erase(id);
        BOOST_ASSERT(running_.size() + terminated_.size() ==
                     result_.processResults.size());
        // collect info
        process::Result &processResult = result_.processResults[id];
        processResult.assign(statLoc);
        if (processResult.completionStatus !=
            process::Result::CompletionStatus::TERMINATED_BY_SYSTEM)
        {
            processResult.completionStatus = process::Result::CompletionStatus::OK;
        }
        if (!static_cast<system::unistd::ProcessResult &>(processResult))
        {
            if (processResult.completionStatus !=
                process::Result::CompletionStatus::TERMINATED_BY_SYSTEM)
            {
                processResult.completionStatus =
                    process::Result::CompletionStatus::ABNORMAL_EXIT;
            }
            // may be overwritten after signal or rusage checks (except START_FAILED)
            if (processResult.termSig)
            {
                switch (processResult.termSig.get())
                {
                case SIGXFSZ:
                    processResult.completionStatus =
                        process::Result::CompletionStatus::OUTPUT_LIMIT_EXCEEDED;
                    break;
                default:
                    // not constexpr
                    if (processResult.termSig.get() == SIG_START_FAILED)
                    {
                        processResult.completionStatus =
                            process::Result::CompletionStatus::START_FAILED;
                    }
                }
            }
        }

        switch (processResult.completionStatus)
        {
        // if !START_FAILED data is meaningful,
        // moreover, START_FAILED should not be rewritten
        case process::Result::CompletionStatus::TERMINATED_BY_SYSTEM:
        case process::Result::CompletionStatus::ABNORMAL_EXIT:
        case process::Result::CompletionStatus::OK:
            collectResourceInfo(processInfo);
            break;
        default:
            ; // do nothing
        }

        signals_.termination(processInfo.meta(), processResult);

        // group checks
        if (result_.processGroupResult.completionStatus ==
            process_group::Result::CompletionStatus::OK)
        {
            if (!processResult &&
                terminateGroupOnCrash_.find(id) != terminateGroupOnCrash_.end())
            {
                result_.processGroupResult.completionStatus =
                    process_group::Result::CompletionStatus::ABNORMAL_EXIT;
            }
        }
    }

    void ExecutionMonitor::terminatedBySystem(ProcessInfo &processInfo)
    {
        const std::size_t id = processInfo.id();

        STREAM_TRACE << "Terminated by system " << processInfo << ".";
        result_.processResults[id].completionStatus =
            process::Result::CompletionStatus::TERMINATED_BY_SYSTEM;
    }

    void ExecutionMonitor::allTerminated()
    {
        STREAM_TRACE << "All processes has terminated.";
        signals_.close();
    }

    void ExecutionMonitor::realTimeLimitExceeded()
    {
        STREAM_TRACE << "Real time limit exceeded.";
        result_.processGroupResult.completionStatus =
            process_group::Result::CompletionStatus::REAL_TIME_LIMIT_EXCEEDED;
    }

    bool ExecutionMonitor::runOutOfResourceLimits(ProcessInfo &processInfo)
    {
        STREAM_TRACE << "Check if " << processInfo << " " <<
                        "run out of resource limits.";
        return collectResourceInfo(processInfo) !=
               process::Result::CompletionStatus::OK;
    }

    process::Result::CompletionStatus ExecutionMonitor::collectResourceInfo(
        ProcessInfo &processInfo)
    {
        const std::size_t id = processInfo.id();

        STREAM_TRACE << "Collecting resource info for " << processInfo << ".";
        process::Result &result = result_.processResults[id];
        process::Result::CompletionStatus &status = result.completionStatus;
        BOOST_ASSERT(status != process::Result::CompletionStatus::START_FAILED);
        process::ResourceUsage &resourceUsage = result.resourceUsage;
        const process::ResourceLimits &resourceLimits = resourceLimits_[id];
        processInfo.fillResourceUsage(resourceUsage);
        if (resourceUsage.timeUsage > resourceLimits.timeLimit)
        {
            STREAM_TRACE << processInfo << " run out of time limit.";
            return status = process::Result::CompletionStatus::TIME_LIMIT_EXCEEDED;
        }
        if (resourceUsage.userTimeUsage > resourceLimits.userTimeLimit)
        {
            STREAM_TRACE << processInfo << " run out of user time limit.";
            return status =
                process::Result::CompletionStatus::USER_TIME_LIMIT_EXCEEDED;
        }
        if (resourceUsage.systemTimeUsage > resourceLimits.systemTimeLimit)
        {
            STREAM_TRACE << processInfo << " run out of system time limit.";
            return status =
                process::Result::CompletionStatus::SYSTEM_TIME_LIMIT_EXCEEDED;
        }
        if (resourceUsage.memoryUsageBytes > resourceLimits.memoryLimitBytes)
        {
            STREAM_TRACE << processInfo << " run out of memory limit.";
            return status =
                process::Result::CompletionStatus::MEMORY_LIMIT_EXCEEDED;
        }
        // note: do not overwrite by OK
        return process::Result::CompletionStatus::OK;
    }

    const AsyncProcessGroup::Result &ExecutionMonitor::result() const
    {
        BOOST_ASSERT(!processesAreRunning());
        BOOST_ASSERT(running_.empty());
        BOOST_ASSERT(running_.size() + terminated_.size() ==
                     result_.processResults.size());
        BOOST_ASSERT(terminated_.size() == result_.processResults.size());
        return result_;
    }
}}}}}}
