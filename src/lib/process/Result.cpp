#include <yandex/contest/invoker/process/Result.hpp>

#include <yandex/contest/invoker/process/ResourceLimits.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process
{
    Result::Result(const int statLoc):
        system::unistd::ProcessResult(statLoc) {}

    Result::operator bool() const
    {
        return system::unistd::ProcessResult::operator bool() &&
               completionStatus == CompletionStatus::OK;
    }
}}}}
