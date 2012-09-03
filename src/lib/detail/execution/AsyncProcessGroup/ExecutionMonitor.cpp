#include "ExecutionMonitor.hpp"

// SIG_START_FAILED
#include "ProcessStarter.hpp"

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

    void ExecutionMonitor::terminated(const Id id, const int statLoc, const ::rusage &rusage_)
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
        process::ResourceLimits &resourceLimits = resourceLimits_[id];
        processResult.assign(statLoc, rusage_);
        if (!processResult)
        {
            // may be overwritten after signal or rusage checks
            if (processResult.termSig && processResult.termSig.get() == SIG_START_FAILED)
                processResult.completionStatus = process::Result::CompletionStatus::START_FAILED;
            else
                processResult.completionStatus = process::Result::CompletionStatus::ABNORMAL_EXIT;
        }
        // if !START_FAILED data is meaningful
        if (processResult.completionStatus != process::Result::CompletionStatus::START_FAILED)
        {
            // special signals
            if (processResult.termSig)
            {
                switch (processResult.termSig.get())
                {
                case SIGXCPU:
                    processResult.completionStatus =
                        process::Result::CompletionStatus::TIME_LIMIT_EXCEEDED;
                    break;
                case SIGXFSZ:
                    processResult.completionStatus =
                        process::Result::CompletionStatus::OUTPUT_LIMIT_EXCEEDED;
                    break;
                }
            }
            if (processResult.completionStatus == process::Result::CompletionStatus::OK ||
                    processResult.completionStatus == process::Result::CompletionStatus::ABNORMAL_EXIT)
                processResult.checkResourceUsage(resourceLimits);
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

    const AsyncProcessGroup::Result &ExecutionMonitor::result() const
    {
        BOOST_ASSERT(!processesAreRunning());
        BOOST_ASSERT(running_.empty());
        BOOST_ASSERT(running_.size() + terminated_.size() == result_.processResults.size());
        BOOST_ASSERT(terminated_.size() == result_.processResults.size());
        return result_;
    }
}}}}}}
