#pragma once

#include <yandex/contest/invoker/ContainerError.hpp>
#include <yandex/contest/invoker/detail/execution/AsyncProcessGroupDetail.hpp>
#include <yandex/contest/invoker/Error.hpp>

#include <yandex/contest/system/execution/AsyncProcess.hpp>
#include <yandex/contest/system/execution/ResultError.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {

using namespace system::execution;

struct AsyncProcessGroupError : virtual Error {};

struct AsyncProcessGroupControlProcessError : ContainerUtilityError,
                                              virtual AsyncProcessGroupError {
  using ContainerUtilityError::ContainerUtilityError;
};

class AsyncProcessGroup {
 public:
  using AccessMode = async_process_group_detail::AccessMode;
  using File = async_process_group_detail::File;
  using Pipe = async_process_group_detail::Pipe;
  using FdAlias = async_process_group_detail::FdAlias;
  using NotificationStream = async_process_group_detail::NotificationStream;
  using Process = async_process_group_detail::Process;
  using ProcessMeta = async_process_group_detail::ProcessMeta;
  using Stream = async_process_group_detail::Stream;
  using NonPipeStream = async_process_group_detail::NonPipeStream;
  using Task = async_process_group_detail::Task;
  using Result = async_process_group_detail::Result;

 public:
  /// Invalid AsyncProcessGroup instance.
  AsyncProcessGroup() = default;

  /*!
   * \brief Start new asynchronous group asynchronous.
   *
   * \param options Settings for control process,
   * AsyncProcess::Options::in will be redefined.
   * \param task Process group settings.
   */
  AsyncProcessGroup(const AsyncProcess::Options &options, const Task &task);
  AsyncProcessGroup(const AsyncProcessGroup &) = delete;
  AsyncProcessGroup(AsyncProcessGroup &&);
  AsyncProcessGroup &operator=(const AsyncProcessGroup &) = delete;
  AsyncProcessGroup &operator=(AsyncProcessGroup &&);

  /// If AsyncProcessGroup instance is valid.
  explicit operator bool() const noexcept;

  /// Calls stop().
  ~AsyncProcessGroup();

  /*!
   * \brief Wait for process group termination.
   *
   * Set execution result.
   */
  const Result &wait();

  /*!
   * \brief Check if process group has terminated.
   *
   * Set execution result if terminated.
   *
   * \return Initialized execution result if process
   * has terminated.
   */
  const boost::optional<Result> &poll();

  /// If process group has not terminated try to kill and wait.
  void stop();

  void swap(AsyncProcessGroup &processGroup) noexcept;

 public:
  /*!
   * \brief Control process implementation.
   *
   * \warning Should be called from separate process.
   * Function assumes that application is single-threaded
   * and no functions except this spawns processes.
   */
  static Result execute(const Task &task);

 private:
  void readResult();

 private:
  AsyncProcess controlProcess_;
  boost::optional<Result> result_;
};

inline void swap(AsyncProcessGroup &a, AsyncProcessGroup &b) noexcept {
  a.swap(b);
}

}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
