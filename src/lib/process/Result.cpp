#include "yandex/contest/invoker/process/Result.hpp"
#include "yandex/contest/invoker/process/ResourceLimits.hpp"

namespace yandex{namespace contest{namespace invoker{namespace process
{
    Result::Result(const int statLoc, const ::rusage &rusage):
        system::unistd::ProcessResult(statLoc),
        resourceUsage(rusage) {}

    void Result::assign(const int statLoc, const ::rusage &rusage)
    {
        system::unistd::ProcessResult::assign(statLoc);
        resourceUsage.assign(rusage);
    }

    void Result::checkResourceUsage(const ResourceLimits &resourceLimits)
    {
        if (resourceUsage.memoryUsageBytes > resourceLimits.memoryLimitBytes)
        {
            completionStatus = CompletionStatus::MEMORY_LIMIT_EXCEEDED;
            return;
        }
        if (resourceUsage.timeUsageMillis > resourceLimits.timeLimitMillis)
        {
            completionStatus = CompletionStatus::TIME_LIMIT_EXCEEDED;
            return;
        }
    }

    Result::operator bool() const
    {
        return system::unistd::ProcessResult::operator bool() &&
               completionStatus == CompletionStatus::OK;
    }
}}}}
