#pragma once

#include <yandex/contest/invoker/ContainerError.hpp>
#include <yandex/contest/invoker/Forward.hpp>
#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>
#include <yandex/contest/invoker/process/DefaultSettings.hpp>
#include <yandex/contest/invoker/process_group/ResourceLimits.hpp>
#include <yandex/contest/invoker/process_group/Result.hpp>
#include <yandex/contest/invoker/process_group/State.hpp>

#include <yandex/contest/IntrusivePointeeBase.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

namespace yandex {
namespace contest {
namespace invoker {

struct ProcessGroupError : virtual ContainerError {};

struct ProcessGroupIllegalStateError : virtual ProcessGroupError,
                                       virtual ContainerIllegalStateError {};

struct ProcessGroupHasAlreadyStartedError
    : virtual ProcessGroupIllegalStateError {};

struct ProcessGroupHasNotStartedError : virtual ProcessGroupIllegalStateError {
};

struct ProcessGroupHasAlreadyTerminatedError
    : virtual ProcessGroupIllegalStateError {};

struct ProcessGroupHasNotTerminatedError
    : virtual ProcessGroupIllegalStateError {};

struct ProcessGroupNotifierError : virtual ProcessGroupError {
  using notifierId = boost::error_info<struct notifierIdTag, std::size_t>;
};

struct ProcessGroupNotifierOutOfRangeError : virtual ProcessGroupNotifierError {
};

struct ProcessGroupNotifierIllegalSinkError
    : virtual ProcessGroupNotifierError {};

namespace process_group {
struct DefaultSettings;
}  // namespace process_group

class ProcessGroup : public IntrusivePointeeBase {
 public:
  using Result = process_group::Result;
  using ResourceLimits = process_group::ResourceLimits;
  using ResourceUsage = process_group::ResourceUsage;
  using DefaultSettings = process_group::DefaultSettings;
  using State = process_group::State;

 public:
  static ProcessGroupPointer create(const ContainerPointer &container);
  ~ProcessGroup();

  /*!
   * \brief Start all processes, associated with process group.
   *
   * It is not possible to start multiple process groups,
   * associated with the same container.
   *
   * \throws ProcessGroupIllegalStateError
   * if any other process group is running.
   */
  void start();

  /*!
   * \brief Stop all processes, started by process group.
   *
   * This function kills all associated processes
   * and controlling process.
   * All CompletionStatus fields will be set STOPPED.
   */
  void stop();

  /*!
   * \brief Start process group and wait for termination.
   *
   * \see start()
   * \see wait()
   */
  const Result &synchronizedCall();

  /*!
   * \brief Check if process group has terminated.
   *
   * Set process group result if terminated.
   *
   * \return Initialized process group result
   * if process group has terminated.
   */
  boost::optional<Result> poll();

  /*!
   * \brief Wait for process group termination.
   * Set process group result.
   */
  const Result &wait();

  /*!
   * \return ProcessGroupResult previously set by poll() or wait().
   *
   * \throws ProcessGroupIllegalStateError
   * if process group result was not set.
   *
   * \deprecated We can use wait() instead.
   */
  const Result &result();

  /*!
   * \return Current state of process group.
   */
  State state();

  /*!
   * \brief Create new process, associated with ProcessGroup.
   *
   * \throws ProcessGroupIllegalStateError
   * if process group has already started.
   *
   * \see start()
   */
  ProcessPointer createProcess(const boost::filesystem::path &executable);

  /*!
   * \brief Create new pipe, associated with ProcessGroup.
   *
   * \throws ProcessGroupIllegalStateError
   * if process group has already started.
   */
  Pipe createPipe();

  const ResourceLimits &resourceLimits() const;
  void setResourceLimits(const ResourceLimits &resourceLimits);

  /*!
   * \brief Process with other pipe end will receive
   * notifications that can be accessed by Notifier.
   *
   * \throws ProcessGroupNotifierIllegalSinkError
   * if notificationStream.pipeEnd.end != WRITE
   */
  NotificationStream notifier(std::size_t notifierId) const;

  void setNotifier(std::size_t notifierId,
                   const NotificationStream &notificationStream);
  void setNotifier(std::size_t notifierId, const Pipe::End &pipeEnd,
                   NotificationStream::Protocol protocol);
  void setNotifier(std::size_t notifierId, const Pipe::End &pipeEnd);

  std::size_t addNotifier(const NotificationStream &notificationStream);
  std::size_t addNotifier(const Pipe::End &pipeEnd,
                          NotificationStream::Protocol protocol);
  std::size_t addNotifier(const Pipe::End &pipeEnd);
  Pipe::End addNotifier(NotificationStream::Protocol protocol);
  Pipe::End addNotifier();

  /*!
   * \brief Default settings for process.
   *
   * Will be used by every created instance of Process.
   *
   * \see ProcessDefaultSettings
   */
  const process::DefaultSettings &processDefaultSettings() const;

  /*!
   * \brief Set process default settings.
   *
   * \see processDefaultSettings()
   */
  void setProcessDefaultSettings(
      const process::DefaultSettings &processDefaultSettings);

 private:
  /*!
   * \warning Constructor is private because
   * class uses own reference-counting mechanism.
   * Lifetime of ProcessGroup object
   * is depended on lifetime of associated Process objects.
   */
  explicit ProcessGroup(const ContainerPointer &container);

  friend class Process;

  ProcessTask &processTask(std::size_t id);
  const process::Result &processResult(std::size_t id);

 private:
  /// If pointer is null process group has terminated.
  ContainerPointer container_;
  /// If processGroup_ is not valid ProcessGroup was not started.
  detail::execution::AsyncProcessGroup processGroup_;
  detail::execution::AsyncProcessGroup::Task task_;
  boost::optional<detail::execution::AsyncProcessGroup::Result> result_;
  process::DefaultSettings processDefaultSettings_;
};

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
