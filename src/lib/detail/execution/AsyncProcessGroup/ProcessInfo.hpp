#pragma once

#include <yandex/contest/system/cgroup/ControlGroup.hpp>

#include <sys/types.h>

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
     * \note Defined in ProcessStarter.cpp
     *
     * \warning When ProcessResult::completionStatus ==
     * ProcessResult::CompletionStatus::START_FAILED other fields
     * have unspecified values (e.g. user should not assume that termSig is set).
     */
    extern const int SIG_START_FAILED;

    struct ProcessInfo
    {
        Pid pid;
        system::cgroup::ControlGroup controlGroup;
    };
}}}}}}
