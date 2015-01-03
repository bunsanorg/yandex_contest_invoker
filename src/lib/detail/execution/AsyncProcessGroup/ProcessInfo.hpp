#pragma once

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>

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
        const ProcessMeta &meta() const;
        void setMeta(const ProcessMeta &meta);

        std::size_t id() const;
        void setId(const std::size_t id);

        const std::string &name() const;
        void setName(const std::string &name);

        Pid pid() const;
        void setPid(const Pid &pid);

        const system::cgroup::ControlGroup &controlGroup() const;
        system::cgroup::ControlGroup &controlGroup();
        void setControlGroup(const system::cgroup::ControlGroupPointer &controlGroup);
        void unsetControlGroup();

        void terminate();
        bool terminated() const;

        void fillResourceUsage(process::ResourceUsage &resourceUsage) const;
        void fillTimeUsage(process::ResourceUsage &resourceUsage) const;

        std::uint64_t maxMemoryUsageBytes() const;
        void updateMaxMemoryUsageFromMemoryStat();

    private:
        void updateMaxMemoryUsageBytes(const std::uint64_t &memoryUsageBytes);
        bool setMaxMemoryUsageBytesIfZero(const std::uint64_t &memoryUsageBytes);

        friend std::ostream &operator<<(std::ostream &out, const ProcessInfo &info);

    private:
        ProcessMeta meta_;
        Pid pid_{0};
        system::cgroup::ControlGroupPointer controlGroup_;
        std::atomic<bool> terminated_{false};
        std::atomic<std::uint64_t> maxMemoryUsageBytes_{0};
    };

    std::ostream &operator<<(std::ostream &out, const ProcessInfo &info);
}}}}}}
