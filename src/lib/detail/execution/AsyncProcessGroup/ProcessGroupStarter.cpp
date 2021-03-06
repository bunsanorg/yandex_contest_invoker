#include "ProcessGroupStarter.hpp"

#include <yandex/contest/system/cgroup/MultipleControlGroup.hpp>
#include <yandex/contest/system/unistd/Operations.hpp>

#include <yandex/contest/StreamLog.hpp>
#include <yandex/contest/SystemError.hpp>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>

#include <functional>
#include <set>

#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

namespace {
void nop(int) {}
}  // namespace

const ProcessGroupStarter::Duration ProcessGroupStarter::waitInterval =
    std::chrono::duration_cast<ProcessGroupStarter::Duration>(
        std::chrono::milliseconds(100));

system::cgroup::ControlGroupPointer ProcessGroupStarter::getThisCgroup() {
  const char *const subsystems[] = {"cpuacct", "cpuset", "freezer", "memory"};
  const auto sys = system::cgroup::SystemInfo::instance();
  std::set<std::size_t> hierarchies;
  for (const char *const subsystem : subsystems)
    hierarchies.insert(sys->bySubsystem(subsystem).id);
  return system::cgroup::MultipleControlGroup::forSelf(hierarchies.begin(),
                                                       hierarchies.end());
}

ProcessGroupStarter::ProcessGroupStarter(const AsyncProcessGroup::Task &task)
    : work_(ioService_),
      thisCgroup_(getThisCgroup()),
      id2processInfo_(task.processes.size()),
      notifiers_(task.notifiers.size()),
      monitor_(task.processes) {
  workers_.create_thread(
      boost::bind(&boost::asio::io_service::run, &ioService_));

  std::vector<system::unistd::Pipe> pipes_(task.pipesNumber);

  // notifiers setup
  for (std::size_t notifierId = 0; notifierId < task.notifiers.size();
       ++notifierId) {
    const NotificationStream &notificationStream = task.notifiers[notifierId];
    const Pipe::End &pipeEnd = notificationStream.pipeEnd;
    BOOST_ASSERT(pipeEnd.end == Pipe::End::WRITE);
    BOOST_ASSERT(pipeEnd.pipeId < pipes_.size());
    STREAM_TRACE << "Allocating notifier " << notifierId << "...";
    auto notifier = notifiers_[notifierId] = boost::make_shared<Notifier>(
        ioService_, pipes_[pipeEnd.pipeId].releaseWriteEnd().release(),
        notificationStream.protocol);
    STREAM_TRACE << "Notifier " << notifierId << " "
                 << "was successfully allocated, configuring...";
    monitor_.onSpawn(
        Notifier::SpawnSignal::slot_type(
            boost::bind(&Notifier::spawn, notifier.get(), _1)).track(notifier));
    monitor_.onTermination(
        Notifier::TerminationSignal::slot_type(
            boost::bind(&Notifier::termination, notifier.get(), _1, _2))
            .track(notifier));
    monitor_.onClose(
        Notifier::CloseSignal::slot_type(
            boost::bind(&Notifier::close, notifier.get())).track(notifier));
    STREAM_TRACE << "Notifier " << notifierId << " "
                 << "was successfully configured.";
  }

  // processes setup
  // TODO restrict memory usage of process group (excluding control process)
  for (std::size_t id = 0; id < task.processes.size(); ++id) {
    BOOST_ASSERT(task.processes[id].meta.id == id);
    const std::string cid = str(boost::format("id_%1%") % id);
    // we don't children to have access to cgroups
    system::cgroup::ControlGroupPointer cg =
        thisCgroup_->createChild(cid, 0700);
    id2processInfo_[id].setControlGroup(cg);
    ProcessStarter starter(cg, task.processes[id], pipes_);
    const Pid pid = starter();
    id2processInfo_[id].setMeta(task.processes[id].meta);
    id2processInfo_[id].setPid(pid);
    BOOST_ASSERT(pid2id_.find(pid) == pid2id_.end());
    pid2id_[pid] = id;
    monitor_.started(id2processInfo_[id], task.processes[id]);
  }

  pipes_.clear();
  BOOST_ASSERT(monitor_.running().size() == task.processes.size());

  // signals setup
  {
    struct sigaction act;

    // SIGALRM
    act.sa_handler = nop;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGALRM, &act, nullptr) < 0)
      BOOST_THROW_EXCEPTION(SystemError("sigaction"));

    // SIGPIPE
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    if (sigaction(SIGPIPE, &act, nullptr) < 0)
      BOOST_THROW_EXCEPTION(SystemError("sigaction"));
  }

  // real time limit
  realTimeLimitPoint_ =
      std::chrono::steady_clock::now() + task.resourceLimits.realTimeLimit;
}

ProcessGroupStarter::~ProcessGroupStarter() {
  // nothing
}

void ProcessGroupStarter::executionLoop() {
  STREAM_DEBUG << "Starting execution loop...";

  workers_.create_thread(
      boost::bind(&ProcessGroupStarter::memoryUsageLoader, this));

  STREAM_TRACE << "Waiting loop...";
  while (monitor_.processGroupIsRunning()) {
    for (const Id id : monitor_.running()) {
      if (monitor_.runOutOfResourceLimits(id2processInfo_[id])) {
        terminate(id);
        monitor_.terminatedBySystem(id2processInfo_[id]);
      }
    }
    waitForAnyChild(std::bind(waitFor, std::placeholders::_1, waitInterval));
    if (Clock::now() >= realTimeLimitPoint_) monitor_.realTimeLimitExceeded();
  }

  // terminate each running process
  STREAM_DEBUG << "Terminating processes...";
  for (const Id id : monitor_.running()) {
    STREAM_TRACE << "Terminating " << id2processInfo_[id] << ".";
    terminate(id);
    monitor_.terminatedBySystem(id2processInfo_[id]);
  }

  // collect results
  STREAM_TRACE << "Collection results...";
  while (monitor_.processesAreRunning()) {
    waitForAnyChild(wait);
  }
  // end of function

  monitor_.allTerminated();

  // FIXME need to execute this block regardless of exceptions
  ioService_.stop();
  STREAM_TRACE << "Joining workers...";
  // everything is terminated, wait for worker threads
  workers_.join_all();

  STREAM_TRACE << "Closing control groups...";
  // let's check everything is OK
  for (ProcessInfo &processInfo : id2processInfo_) {
    BOOST_ASSERT_MSG(processInfo.terminated(),
                     "Every process should be terminated.");
    if (!processInfo.controlGroup().tasks().empty()) {
      STREAM_ERROR << processInfo.controlGroup() << " is not empty!";
      processInfo.terminate();
    }
    processInfo.unsetControlGroup();
  }
  STREAM_TRACE << "Execution loop has completed.";
}

void ProcessGroupStarter::terminate(const Id id) {
  BOOST_ASSERT(id < id2processInfo_.size());
  STREAM_TRACE << "Attempt to terminate " << id2processInfo_[id] << ".";
  id2processInfo_[id].terminate();
}

void ProcessGroupStarter::waitForAnyChild(const WaitFunction &waitFunction) {
  STREAM_TRACE << "Waiting for a child...";
  BOOST_ASSERT(monitor_.processesAreRunning());
  int statLoc;
  const Pid pid = waitFunction(statLoc);
  STREAM_TRACE << "waitFunction has returned pid = " << pid << ".";
  if (pid != 0) {
    BOOST_ASSERT(pid > 0);
    BOOST_ASSERT_MSG(pid2id_.find(pid) != pid2id_.end(),
                     "We received process we haven't started.");
    const Id id = pid2id_.at(pid);
    terminate(id);
    monitor_.terminated(id2processInfo_[id], statLoc);
  }
}

Pid ProcessGroupStarter::wait(int &statLoc) {
  STREAM_TRACE << "Waiting for a child [timeout=infinity]...";
  Pid rpid;
  do {
    rpid = ::wait(&statLoc);
  } while (rpid < 0 && errno == EINTR);
  BOOST_ASSERT_MSG(rpid != 0, "Timeout is not possible.");
  if (rpid < 0) {
    BOOST_ASSERT_MSG(errno != ECHILD, "We have child processes.");
    BOOST_THROW_EXCEPTION(SystemError(errno, "wait")
                          << Error::message("Undocumented error."));
  }
  return rpid;
}

namespace {
template <typename Duration>
::timeval toTimeval(const Duration &duration) {
  constexpr unsigned MAXVAL = 1000 * 1000;
  ::timeval tval;
  tval.tv_sec =
      std::chrono::duration_cast<std::chrono::seconds>(duration).count();
  tval.tv_usec =
      std::chrono::duration_cast<std::chrono::microseconds>(duration).count() %
      MAXVAL;
  BOOST_ASSERT(0 <= tval.tv_sec && tval.tv_sec < MAXVAL);
  BOOST_ASSERT(0 <= tval.tv_usec && tval.tv_usec < MAXVAL);
  return tval;
}  // namespace

/// Used to interrupt system calls.
class IntervalTimer : private boost::noncopyable {
 public:
  template <typename Duration>
  explicit IntervalTimer(const Duration &duration) {
    tval_.it_interval = zeroTimeVal;
    // too small values are dangerous
    tval_.it_value =
        duration < resolution ? toTimeval(resolution) : toTimeval(duration);
    tval_.it_interval = toTimeval(resolution);  // in case event is missed
    BUNSAN_LOG_TRACE << "Setting timer for " << tval_.it_value.tv_sec << "s + "
                     << tval_.it_value.tv_usec << "us";
    system::unistd::setitimer(ITIMER_REAL, tval_);
  }

  void disable() noexcept {
    if (*this) {
      // this function may not throw with zero argument
      system::unistd::setitimer(ITIMER_REAL, zeroITimerVal);
      tval_.it_value = zeroTimeVal;
    }
  }

  explicit operator bool() const noexcept {
    return tval_.it_value.tv_sec || tval_.it_value.tv_usec;
  }

  ~IntervalTimer() { disable(); }

 private:
  static const ::timeval zeroTimeVal;
  static const ::itimerval zeroITimerVal;
  static const std::chrono::milliseconds resolution;

 private:
  ::itimerval tval_;
};

const ::timeval IntervalTimer::zeroTimeVal = {.tv_sec = 0, .tv_usec = 0};

const ::itimerval IntervalTimer::zeroITimerVal = {
    .it_interval = {.tv_sec = 0, .tv_usec = 0},
    .it_value = {.tv_sec = 0, .tv_usec = 0}};

const std::chrono::milliseconds IntervalTimer::resolution(10);
}  // namespace

Pid ProcessGroupStarter::waitFor(int &statLoc, const Duration &duration) {
  STREAM_TRACE << "Waiting for a child [timeout=" << duration.count() << "]...";
  return waitUntil(statLoc, Clock::now() + duration);
}

Pid ProcessGroupStarter::waitUntil(int &statLoc, const TimePoint &untilPoint) {
  STREAM_TRACE << "Waiting for a child [until="
               << untilPoint.time_since_epoch().count() << "]...";
  TimePoint now = Clock::now();
  if (now >= untilPoint) return 0;
  Pid rpid;
  int errno_ = 0;
  do {
    {
      IntervalTimer timer(untilPoint - now);
      // will be interrupted on timer event
      rpid = ::wait(&statLoc);
      errno_ = errno;
    }
    // first assignment and check was made at the beginning of the function
    now = Clock::now();
    if (now >= untilPoint) return rpid > 0 ? rpid : 0;
  } while (rpid < 0 && errno_ == EINTR);
  BOOST_ASSERT_MSG(rpid != 0, "Timeout is not possible.");
  if (rpid < 0) {
    BOOST_ASSERT_MSG(errno_ != ECHILD, "We have child processes.");
    BOOST_THROW_EXCEPTION(SystemError(errno_, "wait")
                          << Error::message("Undocumented error."));
  }
  return rpid;
}

void ProcessGroupStarter::memoryUsageLoader() {
  bool found;
  do {
    found = false;
    for (ProcessInfo &processInfo : id2processInfo_) {
      if (!processInfo.terminated()) {
        found = true;
        processInfo.updateMaxMemoryUsageFromMemoryStat();
      }
    }
    // FIXME hardcode
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
  } while (found);
}

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
