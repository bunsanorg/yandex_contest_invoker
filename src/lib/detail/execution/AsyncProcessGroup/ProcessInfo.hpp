#pragma once

#include <yandex/contest/system/cgroup/ControlGroup.hpp>

#include <atomic>

#include <cstdint>

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

    class ProcessInfo
    {
    public:
        Pid pid() const;
        void setPid(const Pid &pid);

        const system::cgroup::ControlGroup &controlGroup() const;
        system::cgroup::ControlGroup &controlGroup();

        void terminate();
        bool terminated() const;

        std::uint64_t maxMemoryUsageBytes() const;
        void updateMaxMemoryUsageFromMemoryStat();

    private:
        void updateMaxMemoryUsageBytes(const std::uint64_t &memoryUsageBytes);
        bool setMaxMemoryUsageBytesIfZero(const std::uint64_t &memoryUsageBytes);

    private:
        Pid pid_{0};
        system::cgroup::ControlGroup controlGroup_;
        std::atomic<bool> terminated_{false};
        std::atomic<std::uint64_t> maxMemoryUsageBytes_{0};
    };
}}}}}}
