#include "ProcessGroupStarter.hpp"

#include <yandex/contest/SystemError.hpp>

#include <yandex/contest/system/unistd/Operations.hpp>

#include <yandex/contest/detail/LogHelper.hpp>

#include <functional>

#include <boost/assert.hpp>
#include <boost/format.hpp>

#include <signal.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    namespace
    {
        void nop(int) {}
    }

    const ProcessGroupStarter::Duration ProcessGroupStarter::waitInterval =
        std::chrono::duration_cast<ProcessGroupStarter::Duration>(
            std::chrono::milliseconds(100));

    ProcessGroupStarter::ProcessGroupStarter(const AsyncProcessGroup::Task &task):
        thisCgroup_(system::cgroup::ControlGroup::getControlGroup(system::unistd::getpid())),
        id2cgroup_(task.processes.size()),
        id2pid_(task.processes.size()),
        monitor_(task.processes)
    {
        std::vector<system::unistd::Pipe> pipes_(task.pipesNumber);
        for (std::size_t id = 0; id < task.processes.size(); ++id)
        {
            const std::string cid = str(boost::format("id_%1%") % id);
            // we don't children to have access to cgroups
            system::cgroup::ControlGroup &cg =
                id2cgroup_[id] = thisCgroup_.createChild(cid, 0700);
            ProcessStarter starter(cg, task.processes[id], pipes_);
            const Pid pid = starter();
            id2pid_[id] = pid;
            STREAM_TRACE << "Process id mapping was established: " <<
                            "{id=" << id << "} = {pid=" << pid << "}.";
            BOOST_ASSERT(pid2id_.find(pid) == pid2id_.end());
            pid2id_[pid] = id;
            monitor_.started(id, task.processes[id]);
        }
        pipes_.clear();
        BOOST_ASSERT(monitor_.running().size() == task.processes.size());
        // SIGALRM setup
        struct sigaction act;
        act.sa_handler = nop;
        act.sa_flags = 0;
        sigemptyset(&act.sa_mask);
        if (sigaction(SIGALRM, &act, nullptr) < 0)
            BOOST_THROW_EXCEPTION(SystemError("sigaction"));
        // real time limit
        realTimeLimitPoint_ = std::chrono::steady_clock::now() + task.resourceLimits.realTimeLimit;
    }

    ProcessGroupStarter::~ProcessGroupStarter()
    {
        // nothing
    }

    void ProcessGroupStarter::executionLoop()
    {
        STREAM_DEBUG << "Starting execution loop...";
        while (monitor_.processGroupIsRunning())
        {
            for (const Id id: monitor_.running())
            {
                if (monitor_.runOutOfResourceLimits(id, id2cgroup_[id]))
                {
                    terminate(id);
                    monitor_.terminatedBySystem(id);
                }
            }
            waitForAnyChild(std::bind(waitFor, std::placeholders::_1, waitInterval));
            if (Clock::now() >= realTimeLimitPoint_)
                monitor_.realTimeLimitExceeded();
        }
        // let's terminate each running process
        STREAM_DEBUG << "Terminating processes...";
        for (const Id id: monitor_.running())
        {
            STREAM_TRACE << "Terminating " << id << ".";
            terminate(id);
            monitor_.terminatedBySystem(id);
        }
        // let's collect results
        STREAM_TRACE << "Collection results...";
        while (monitor_.processesAreRunning())
        {
            waitForAnyChild(wait);
        }
        // end of function
        // let's check everything is OK
        for (const system::cgroup::ControlGroup &cg: id2cgroup_)
            BOOST_ASSERT_MSG(!cg, "Every control group should be terminated and closed.");
    }

    void ProcessGroupStarter::terminate(const Id id)
    {
        BOOST_ASSERT(id < id2cgroup_.size());
        const Pid pid = id2pid_[id];
        STREAM_TRACE << "Attempt to terminate " << id << " (pid = " << pid << ").";
        // sometimes terminate is called before
        // child has attached itself to control group
        if (::kill(pid, SIGKILL) < 0 && errno != ESRCH)
            BOOST_THROW_EXCEPTION(SystemError("kill") <<
                                  Error::message("This should not happen."));
        id2cgroup_[id].terminate();
    }

    void ProcessGroupStarter::waitForAnyChild(const WaitFunction &waitFunction)
    {
        STREAM_TRACE << "Waiting for a child...";
        BOOST_ASSERT(monitor_.processesAreRunning());
        int statLoc;
        const Pid pid = waitFunction(statLoc);
        STREAM_TRACE << "waitFunction has returned pid = " << pid << ".";
        if (pid != 0)
        {
            BOOST_ASSERT(pid > 0);
            BOOST_ASSERT_MSG(pid2id_.find(pid) != pid2id_.end(), "We get process we haven't started.");
            const Id id = pid2id_.at(pid);
            // TODO Do we need to check for lost children and report?
            // terminate lost children
            terminate(id);
            monitor_.terminated(id, statLoc, id2cgroup_[id]);
            // control group will not be used after that
            id2cgroup_[id].close();
        }
    }

    Pid ProcessGroupStarter::wait(int &statLoc)
    {
        STREAM_TRACE << "Waiting for a child [timeout=infinity]...";
        Pid rpid;
        do
        {
            rpid = ::wait(&statLoc);
        }
        while (rpid < 0 && errno == EINTR);
        BOOST_ASSERT_MSG(rpid != 0, "Timeout is not possible.");
        if (rpid < 0)
        {
            BOOST_ASSERT_MSG(errno != ECHILD, "We have child processes.");
            BOOST_THROW_EXCEPTION(SystemError(errno, "wait") << Error::message("Undocumented error."));
        }
        return rpid;
    }

    namespace
    {
        template <typename Duration>
        ::timeval toTimeval(const Duration &duration)
        {
            constexpr unsigned MAXVAL = 1000 * 1000;
            ::timeval tval;
            tval.tv_sec = std::chrono::duration_cast<
                std::chrono::seconds>(duration).count();
            tval.tv_usec = std::chrono::duration_cast<
                std::chrono::microseconds>(duration).count() % MAXVAL;
            BOOST_ASSERT(0 <= tval.tv_sec && tval.tv_sec < MAXVAL);
            BOOST_ASSERT(0 <= tval.tv_usec && tval.tv_usec < MAXVAL);
            return tval;
        }

        /// Used to interrupt system calls.
        class IntervalTimer: private boost::noncopyable
        {
        public:
            template <typename Duration>
            explicit IntervalTimer(const Duration &duration)
            {
                tval_.it_interval = zeroTimeVal;
                // too small values are dangerous
                tval_.it_value = duration < resolution ?
                    toTimeval(resolution) : toTimeval(duration);
                system::unistd::setitimer(ITIMER_REAL, tval_);
            }

            void disable() noexcept
            {
                if (*this)
                {
                    // this function may not throw with zero argument
                    system::unistd::setitimer(ITIMER_REAL, zeroITimerVal);
                    tval_.it_value = zeroTimeVal;
                }
            }

            explicit operator bool() const noexcept
            {
                return tval_.it_value.tv_sec || tval_.it_value.tv_usec;
            }

            ~IntervalTimer()
            {
                disable();
            }

        private:
            static const ::timeval zeroTimeVal;
            static const ::itimerval zeroITimerVal;
            static const std::chrono::milliseconds resolution;

        private:
            ::itimerval tval_;
        };

        const ::timeval IntervalTimer::zeroTimeVal = {
            .tv_sec = 0, .tv_usec = 0
        };

        const ::itimerval IntervalTimer::zeroITimerVal = {
            .it_interval = {.tv_sec = 0, .tv_usec = 0},
            .it_value = {.tv_sec = 0, .tv_usec = 0}
        };

        const std::chrono::milliseconds IntervalTimer::resolution(10);
    }

    Pid ProcessGroupStarter::waitFor(int &statLoc, const Duration &duration)
    {
        STREAM_TRACE << "Waiting for a child [timeout=" << duration.count() << "]...";
        return waitUntil(statLoc, Clock::now() + duration);
    }

    Pid ProcessGroupStarter::waitUntil(int &statLoc, const TimePoint &untilPoint)
    {
        STREAM_TRACE << "Waiting for a child [until=" <<
                        untilPoint.time_since_epoch().count() << "]...";
        TimePoint now = Clock::now();
        if (now >= untilPoint)
            return 0;
        Pid rpid;
        int errno_ = 0;
        do
        {
            {
                IntervalTimer timer(untilPoint - now);
                // will be interrupted on timer event
                rpid = ::wait(&statLoc);
                errno_ = errno;
            }
            // first assignment and check was made at the beginning of the function
            now = Clock::now();
            if (now >= untilPoint)
                return rpid > 0 ? rpid : 0;
        }
        while (rpid < 0 && errno_ == EINTR);
        BOOST_ASSERT_MSG(rpid != 0, "Timeout is not possible.");
        if (rpid < 0)
        {
            BOOST_ASSERT_MSG(errno_ != ECHILD, "We have child processes.");
            BOOST_THROW_EXCEPTION(SystemError(errno_, "wait") << Error::message("Undocumented error."));
        }
        return rpid;
    }
}}}}}}
