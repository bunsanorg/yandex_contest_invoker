#include "ExecutionMonitor.hpp"

// SIG_START_FAILED
#include "ProcessStarter.hpp"

#include "yandex/contest/system/cgroup/CpuAccounting.hpp"
#include "yandex/contest/system/cgroup/Memory.hpp"
#include "yandex/contest/system/cgroup/MemorySwap.hpp"

#include <signal.h>

#include <boost/assert.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    void ExecutionMonitor::started(const Id id, const AsyncProcessGroup::Process &process)
    {
        BOOST_ASSERT_MSG(running_.find(id) == running_.end(), "This process has already started.");
        running_.insert(id);
        if (process.groupWaitsForTermination)
            groupWaitsForTermination_.insert(id);
        if (process.terminateGroupOnCrash)
            terminateGroupOnCrash_.insert(id);
    }

    void ExecutionMonitor::terminated(const Id id, const int statLoc,
                                      system::cgroup::ControlGroup &controlGroup)
    {
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
        BOOST_ASSERT(running_.size() + terminated_.size() == result_.processResults.size());
        // collect info
        process::Result &processResult = result_.processResults[id];
        processResult.assign(statLoc);
        processResult.completionStatus = process::Result::CompletionStatus::OK;
        if (!processResult)
        {
            processResult.completionStatus = process::Result::CompletionStatus::ABNORMAL_EXIT;
            // may be overwritten after signal or rusage checks (except START_FAILED)
            if (processResult.termSig)
            {
                switch (processResult.termSig.get())
                {
                case SIGXFSZ:
                    processResult.completionStatus = process::Result::CompletionStatus::OUTPUT_LIMIT_EXCEEDED;
                    break;
                default:
                    // not constexpr
                    if (processResult.termSig.get() == SIG_START_FAILED)
                        processResult.completionStatus = process::Result::CompletionStatus::START_FAILED;
                }
            }
        }
        // if !START_FAILED data is meaningful, moreover, START_FAILED should not be rewritten
        if (processResult.completionStatus == process::Result::CompletionStatus::ABNORMAL_EXIT ||
            processResult.completionStatus == process::Result::CompletionStatus::OK)
        {
            collectResourceInfo(id, controlGroup);
        }
        // group checks
        if (result_.processGroupResult.completionStatus == process_group::Result::CompletionStatus::OK)
        {
            if (!processResult && terminateGroupOnCrash_.find(id) != terminateGroupOnCrash_.end())
            {
                result_.processGroupResult.completionStatus =
                    process_group::Result::CompletionStatus::ABNORMAL_EXIT;
            }
        }
    }

    void ExecutionMonitor::realTimeLimitExceeded()
    {
        result_.processGroupResult.completionStatus =
            process_group::Result::CompletionStatus::REAL_TIME_LIMIT_EXCEEDED;
    }

    bool ExecutionMonitor::runOutOfResourceLimits(
        const Id id, system::cgroup::ControlGroup &controlGroup)
    {
        return collectResourceInfo(id, controlGroup) != process::Result::CompletionStatus::OK;
    }

    process::Result::CompletionStatus ExecutionMonitor::collectResourceInfo(
        const Id id, system::cgroup::ControlGroup &controlGroup)
    {
        process::Result &result = result_.processResults[id];
        process::Result::CompletionStatus &status = result.completionStatus;
        BOOST_ASSERT(status != process::Result::CompletionStatus::START_FAILED);
        process::ResourceUsage &resourceUsage = result.resourceUsage;
        const process::ResourceLimits &resourceLimits = resourceLimits_[id];
        const system::cgroup::Memory memory(controlGroup);
        const system::cgroup::MemorySwap memsw(controlGroup);
        const system::cgroup::CpuAccounting cpuAcct(controlGroup);
        resourceUsage.memoryUsageBytes = memory.maxUsage();
        resourceUsage.timeUsageMillis =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                cpuAcct.userUsage()).count();
        if (resourceUsage.timeUsageMillis > resourceLimits.timeLimitMillis)
            return status = process::Result::CompletionStatus::TIME_LIMIT_EXCEEDED;
        if (memory.failcnt() || memsw.failcnt())
            return status = process::Result::CompletionStatus::MEMORY_LIMIT_EXCEEDED;
        // note: do not overwrite by OK
        return process::Result::CompletionStatus::OK;
    }

    const AsyncProcessGroup::Result &ExecutionMonitor::result() const
    {
        BOOST_ASSERT(!processesAreRunning());
        BOOST_ASSERT(running_.empty());
        BOOST_ASSERT(running_.size() + terminated_.size() == result_.processResults.size());
        BOOST_ASSERT(terminated_.size() == result_.processResults.size());
        return result_;
    }
}}}}}}
