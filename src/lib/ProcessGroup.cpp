#include <yandex/contest/invoker/ProcessGroup.hpp>

#include <yandex/contest/invoker/Container.hpp>
#include <yandex/contest/invoker/ContainerError.hpp>
#include <yandex/contest/invoker/lxc/Error.hpp>
#include <yandex/contest/invoker/Process.hpp>

#include <yandex/contest/detail/IntrusivePointerHelper.hpp>

#include <boost/assert.hpp>

#include <thread>

#include <csignal>

namespace yandex {
namespace contest {
namespace invoker {

YANDEX_CONTEST_INTRUSIVE_PTR_DEFINE(ProcessGroup)

ProcessGroup::ProcessGroup(const ContainerPointer &container)
    : container_(container) {
  BOOST_ASSERT(container_);
}

ProcessGroupPointer ProcessGroup::create(const ContainerPointer &container) {
  ProcessGroupPointer ret(new ProcessGroup(container));
  container->processGroupDefaultSettings().setUpProcessGroup(ret);
  return ret;
}

ProcessGroup::~ProcessGroup() {}

ProcessPointer ProcessGroup::createProcess(
    const boost::filesystem::path &executable) {
  if (processGroup_)
    BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyStartedError());
  const std::size_t id = task_.processes.size();
  task_.processes.resize(id + 1);
  processTask(id).meta.id = id;
  processTask(id).executable = executable;
  return Process::create(ProcessGroupPointer(this), id);
}

Pipe ProcessGroup::createPipe() {
  if (processGroup_)
    BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyStartedError());
  return Pipe(task_.pipesNumber++);
}

void ProcessGroup::start() {
  if (processGroup_)
    BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyStartedError());
  processGroup_ = container_->execute(task_);
}

void ProcessGroup::stop() {
  if (!processGroup_) BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
  if (!container_)
    BOOST_THROW_EXCEPTION(ProcessGroupHasAlreadyTerminatedError());
  container_->stop();
  try {
    wait();
  } catch (detail::execution::AsyncProcessGroupControlProcessError &e) {
    // it's OK, stop() caused it
  }
  container_.reset();
  result_ = detail::execution::AsyncProcessGroup::Result();
  result_->processGroupResult.completionStatus =
      ProcessGroup::Result::CompletionStatus::STOPPED;
  result_->processResults.resize(task_.processes.size());
  for (Process::Result &pr : result_->processResults)
    pr.completionStatus = Process::Result::CompletionStatus::STOPPED;
}

const ProcessGroup::Result &ProcessGroup::synchronizedCall() {
  start();
  return wait();
}

boost::optional<ProcessGroup::Result> ProcessGroup::poll() {
  if (!processGroup_) BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
  if (!result_) result_ = processGroup_.poll();
  if (result_) {
    container_.reset();
    return result_->processGroupResult;
  } else {
    return boost::optional<ProcessGroup::Result>();
  }
}

const ProcessGroup::Result &ProcessGroup::wait() {
  if (!processGroup_) BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
  if (!result_) {
    result_ = processGroup_.wait();
    container_.reset();
  }
  BOOST_ASSERT(result_);
  return result_->processGroupResult;
}

const ProcessGroup::Result &ProcessGroup::result() {
  if (!result_) {
    if (container_) {
      BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
    } else {
      BOOST_THROW_EXCEPTION(ProcessGroupHasNotTerminatedError());
    }
  }
  return result_->processGroupResult;
}

const ProcessGroup::ResourceLimits &ProcessGroup::resourceLimits() const {
  return task_.resourceLimits;
}

void ProcessGroup::setResourceLimits(
    const ProcessGroup::ResourceLimits &resourceLimits) {
  task_.resourceLimits = resourceLimits;
}

void ProcessGroup::setNotifier(const std::size_t notifierId,
                               const NotificationStream &notificationStream) {
  if (notificationStream.pipeEnd.end != Pipe::End::WRITE)
    BOOST_THROW_EXCEPTION(ProcessGroupNotifierIllegalSinkError());
  if (notifierId >= task_.notifiers.size()) {
    BOOST_THROW_EXCEPTION(
        ProcessGroupNotifierOutOfRangeError()
        << ProcessGroupNotifierOutOfRangeError::notifierId(notifierId));
  }
  task_.notifiers[notifierId] = notificationStream;
}

void ProcessGroup::setNotifier(const std::size_t notifierId,
                               const Pipe::End &pipeEnd,
                               const NotificationStream::Protocol protocol) {
  setNotifier(notifierId, NotificationStream{pipeEnd, protocol});
}

void ProcessGroup::setNotifier(const std::size_t notifierId,
                               const Pipe::End &pipeEnd) {
  setNotifier(notifierId, pipeEnd, NotificationStream::Protocol::NATIVE);
}

std::size_t ProcessGroup::addNotifier(
    const NotificationStream &notificationStream) {
  const std::size_t notifierId = task_.notifiers.size();
  task_.notifiers.push_back(notificationStream);
  return notifierId;
}

std::size_t ProcessGroup::addNotifier(
    const Pipe::End &pipeEnd, const NotificationStream::Protocol protocol) {
  return addNotifier(NotificationStream{pipeEnd, protocol});
}

std::size_t ProcessGroup::addNotifier(const Pipe::End &pipeEnd) {
  return addNotifier(pipeEnd, NotificationStream::Protocol::NATIVE);
}

Pipe::End ProcessGroup::addNotifier(
    const NotificationStream::Protocol protocol) {
  const Pipe pipe = createPipe();
  addNotifier(pipe.writeEnd(), protocol);
  return pipe.readEnd();
}

Pipe::End ProcessGroup::addNotifier() {
  return addNotifier(NotificationStream::Protocol::NATIVE);
}

NotificationStream ProcessGroup::notifier(const std::size_t notifierId) const {
  if (notifierId >= task_.notifiers.size()) {
    BOOST_THROW_EXCEPTION(
        ProcessGroupNotifierOutOfRangeError()
        << ProcessGroupNotifierOutOfRangeError::notifierId(notifierId));
  }
  return task_.notifiers[notifierId];
}

const process::DefaultSettings &ProcessGroup::processDefaultSettings() const {
  return processDefaultSettings_;
}

void ProcessGroup::setProcessDefaultSettings(
    const process::DefaultSettings &processDefaultSettings) {
  processDefaultSettings_ = processDefaultSettings;
}

ProcessTask &ProcessGroup::processTask(const std::size_t id) {
  BOOST_ASSERT(id < task_.processes.size());
  return task_.processes[id];
}

const process::Result &ProcessGroup::processResult(const std::size_t id) {
  if (!result_) {
    if (container_) {
      BOOST_THROW_EXCEPTION(ProcessGroupHasNotStartedError());
    } else {
      BOOST_THROW_EXCEPTION(ProcessGroupHasNotTerminatedError());
    }
  }
  BOOST_ASSERT(result_->processResults.size() == task_.processes.size());
  BOOST_ASSERT(id < result_->processResults.size());
  return result_->processResults[id];
}

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
