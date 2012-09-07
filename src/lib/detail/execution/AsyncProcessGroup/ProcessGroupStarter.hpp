#pragma once

#include "yandex/contest/system/unistd/Pipe.hpp"

#include "yandex/contest/system/cgroup/ControlGroup.hpp"

#include "ExecutionMonitor.hpp"
#include "ProcessStarter.hpp"

#include <chrono>
#include <functional>

#include <boost/noncopyable.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    class ProcessGroupStarter: private boost::noncopyable
    {
    public:
        typedef std::chrono::steady_clock Clock;
        typedef Clock::time_point TimePoint;
        typedef Clock::duration Duration;

        /*!
         * \brief Wait for child.
         *
         * \return negative on fail, zero on skip, positive on success
         */
        typedef std::function<Pid (int &)> WaitFunction;

    public:
        explicit ProcessGroupStarter(const AsyncProcessGroup::Task &task);

        void executionLoop();

        const AsyncProcessGroup::Result &result() const { return monitor_.result(); }

    private:
        void terminate(const Id id);

        void waitForAnyChild(const WaitFunction &waitFunction);

        /// wait3 analogue except it handles interruptions
        static Pid wait(int &statLoc);

        /// Return 0 if no process has terminated during duration time
        static Pid waitFor(int &statLoc, const Duration &duration);

        /*!
         * \brief wait3 analogue except it handles interruptions.
         *
         * \return 0 if until was reached
         */
        static Pid waitUntil(int &statLoc, const TimePoint &untilPoint);

    private:
        static const Duration waitInterval;

    private:
        system::cgroup::ControlGroup thisCgroup_;
        std::vector<system::cgroup::ControlGroup> id2cgroup_;
        std::unordered_map<Pid, Id> pid2id_;
        std::vector<Pid> id2pid_;
        ExecutionMonitor monitor_;
        process_group::ResourceLimits resourceLimits_;
        TimePoint realTimeLimitPoint_;
    };
}}}}}}
