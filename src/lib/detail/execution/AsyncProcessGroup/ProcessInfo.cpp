#include "ProcessInfo.hpp"

#include <yandex/contest/system/cgroup/CpuAccounting.hpp>
#include <yandex/contest/system/cgroup/Memory.hpp>
#include <yandex/contest/system/cgroup/MemorySwap.hpp>
#include <yandex/contest/system/cgroup/Termination.hpp>

#include <yandex/contest/StreamLog.hpp>
#include <yandex/contest/SystemError.hpp>

#include <boost/assert.hpp>

#include <signal.h>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    const ProcessMeta &ProcessInfo::meta() const
    {
        return meta_;
    }

    void ProcessInfo::setMeta(const ProcessMeta &meta)
    {
        meta_ = meta;
    }

    std::size_t ProcessInfo::id() const
    {
        return meta_.id;
    }

    void ProcessInfo::setId(const std::size_t id)
    {
        meta_.id = id;
    }

    const std::string &ProcessInfo::name() const
    {
        return meta_.name;
    }

    void ProcessInfo::setName(const std::string &name)
    {
        meta_.name = name;
    }

    Pid ProcessInfo::pid() const
    {
        return pid_;
    }

    void ProcessInfo::setPid(const Pid &pid)
    {
        pid_ = pid;
    }

    const system::cgroup::ControlGroup &ProcessInfo::controlGroup() const
    {
        BOOST_ASSERT(controlGroup_);
        return *controlGroup_;
    }

    system::cgroup::ControlGroup &ProcessInfo::controlGroup()
    {
        BOOST_ASSERT(controlGroup_);
        return *controlGroup_;
    }

    void ProcessInfo::setControlGroup(
        const system::cgroup::ControlGroupPointer &controlGroup)
    {
        controlGroup_ = controlGroup;
        terminationGuard_ = system::cgroup::TerminationGuard(controlGroup_);
    }

    void ProcessInfo::unsetControlGroup()
    {
        controlGroup_.reset();
        terminationGuard_ = system::cgroup::TerminationGuard();
    }

    void ProcessInfo::terminate()
    {
        STREAM_TRACE << "Attempt to terminate {pid = " << pid_ << "}";
        // sometimes terminate is called before
        // child has attached itself to control group
        if (::kill(pid_, SIGKILL) < 0 && errno != ESRCH)
            BOOST_THROW_EXCEPTION(SystemError("kill") <<
                                  Error::message("This should not happen."));
        // \todo Do we need to check for lost children and report?
        system::cgroup::terminate(controlGroup_);
        terminated_.store(true);

        // collect memory usage info
        const system::cgroup::Memory memory(controlGroup_);
        // this happens if memory usage has never been taken from stat()
        // while process was alive, in that case maxUsage() is pretty accurate
        // TODO take memsw.maxUsage() into account
        setMaxMemoryUsageBytesIfZero(memory.maxUsage());
    }

    bool ProcessInfo::terminated() const
    {
        return terminated_.load();
    }

    void ProcessInfo::fillResourceUsage(
        process::ResourceUsage &resourceUsage) const
    {
        resourceUsage.memoryUsageBytes = maxMemoryUsageBytes();
        fillTimeUsage(resourceUsage);
    }

    void ProcessInfo::fillTimeUsage(
        process::ResourceUsage &resourceUsage) const
    {
        const system::cgroup::CpuAccounting cpuAcct(controlGroup_);
        const auto cpuAcctStat = cpuAcct.stat();
        resourceUsage.userTimeUsage =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                cpuAcctStat.userUsage);
        resourceUsage.systemTimeUsage =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                cpuAcctStat.systemUsage);
        resourceUsage.timeUsage =
            std::chrono::duration_cast<std::chrono::nanoseconds>(cpuAcct.usage());
    }

    std::uint64_t ProcessInfo::maxMemoryUsageBytes() const
    {
        return maxMemoryUsageBytes_.load();
    }

    void ProcessInfo::updateMaxMemoryUsageFromMemoryStat()
    {
        const system::cgroup::Memory memory(controlGroup_);
        const system::cgroup::MemorySwap memsw(controlGroup_);
        // TODO take swap into account
        updateMaxMemoryUsageBytes(memory.stat().at("rss"));
    }

    void ProcessInfo::updateMaxMemoryUsageBytes(
        const std::uint64_t &memoryUsageBytes)
    {
        std::uint64_t expected = 0;
        // lock-free maxMemoryUsage update
        while (expected < memoryUsageBytes &&
               !maxMemoryUsageBytes_.compare_exchange_weak(
                    expected, memoryUsageBytes))
            continue;
    }

    bool ProcessInfo::setMaxMemoryUsageBytesIfZero(
        const std::uint64_t &memoryUsageBytes)
    {
        std::uint64_t expected = 0;
        return maxMemoryUsageBytes_.compare_exchange_strong(
            expected, memoryUsageBytes);
    }

    std::ostream &operator<<(std::ostream &out, const ProcessInfo &info)
    {
        return out << info.meta_;
    }
}}}}}}
