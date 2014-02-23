#include "ProcessInfo.hpp"

#include <yandex/contest/SystemError.hpp>

#include <signal.h>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
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
        return controlGroup_;
    }

    system::cgroup::ControlGroup &ProcessInfo::controlGroup()
    {
        return controlGroup_;
    }

    void ProcessInfo::terminate()
    {
        // sometimes terminate is called before
        // child has attached itself to control group
        if (::kill(pid_, SIGKILL) < 0 && errno != ESRCH)
            BOOST_THROW_EXCEPTION(SystemError("kill") <<
                                  Error::message("This should not happen."));
        // \todo Do we need to check for lost children and report?
        controlGroup_.terminate();
        terminated_.store(true);
    }

    bool ProcessInfo::terminated() const
    {
        return terminated_.load();
    }
}}}}}}
