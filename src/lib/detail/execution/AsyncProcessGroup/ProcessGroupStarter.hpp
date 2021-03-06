#pragma once

#include "ExecutionMonitor.hpp"
#include "Notifier.hpp"
#include "ProcessInfo.hpp"
#include "ProcessStarter.hpp"

#include <yandex/contest/system/cgroup/ControlGroup.hpp>
#include <yandex/contest/system/unistd/Pipe.hpp>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

class ProcessGroupStarter : private boost::noncopyable {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  /*!
   * \brief Wait for child.
   *
   * \return negative on fail, zero on skip, positive on success
   */
  using WaitFunction = std::function<Pid(int &)>;

 public:
  explicit ProcessGroupStarter(const AsyncProcessGroup::Task &task);

  ~ProcessGroupStarter();

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
   *
   * \warning Function may return pid even if untilPoint was reached.
   */
  static Pid waitUntil(int &statLoc, const TimePoint &untilPoint);

  void memoryUsageLoader();

 private:
  static const Duration waitInterval;

  static system::cgroup::ControlGroupPointer getThisCgroup();

 private:
  boost::asio::io_service ioService_;
  boost::asio::io_service::work work_;

  boost::thread_group workers_;

  system::cgroup::ControlGroupPointer thisCgroup_;
  std::vector<ProcessInfo> id2processInfo_;
  std::unordered_map<Pid, Id> pid2id_;

  std::vector<boost::shared_ptr<Notifier>> notifiers_;

  ExecutionMonitor monitor_;
  process_group::ResourceLimits resourceLimits_;
  TimePoint realTimeLimitPoint_;
};

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
