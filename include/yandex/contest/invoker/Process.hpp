#pragma once

#include <yandex/contest/invoker/ContainerError.hpp>
#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>
#include <yandex/contest/invoker/Forward.hpp>
#include <yandex/contest/invoker/process/ResourceLimits.hpp>
#include <yandex/contest/invoker/process/Result.hpp>

#include <yandex/contest/system/execution/Helper.hpp>

#include <yandex/contest/IntrusivePointeeBase.hpp>

#include <boost/filesystem/path.hpp>

namespace yandex {
namespace contest {
namespace invoker {

struct ProcessError : virtual ContainerError {};

struct ProcessDescriptorError : virtual ProcessError {
  using descriptor = boost::error_info<struct descriptorTag, int>;
};

struct ProcessDescriptorOutOfRangeError : virtual ProcessDescriptorError {};

struct ProcessEnvironmentError : virtual ProcessError {
  using key = boost::error_info<struct keyTag, std::string>;
};
struct ProcessUndefinedEnvironmentVariableError
    : virtual ProcessEnvironmentError {};

namespace process {
struct DefaultSettings;
}  // namespace process

/*!
 * \todo This class may copyable.
 * ProcessPointer is possibly excess class.
 * It should be checked. -- ProcessHandle
 */
class Process : public IntrusivePointeeBase {
 public:
  using Result = process::Result;
  using ResourceLimits = process::ResourceLimits;
  using ResourceUsage = process::ResourceUsage;
  using DefaultSettings = process::DefaultSettings;

  using Id = std::size_t;

 public:
  /*!
   * \brief Create new Process, associated with processGroup.
   * TODO: lifetime
   */
  static ProcessPointer create(const ProcessGroupPointer &processGroup, Id id);

  const boost::filesystem::path &executable() const;

  /*!
   * ProcessGroup is running while at least
   * on of processes marked with that flag
   * is running.
   */
  bool groupWaitsForTermination() const;

  /*!
   * \see groupWaitsForTermination()
   * \note Default value is true.
   */
  void setGroupWaitsForTermination(bool groupWaitsForTermination = true);

  /*!
   * ProcessGroup will terminate
   * if one of processes marked with this flag
   * has crashed (completion status is not OK).
   */
  bool terminateGroupOnCrash() const;

  /*!
   * \see terminateGroupOnCrash()
   * \note Default value is true.
   */
  void setTerminateGroupOnCrash(bool terminateGroupOnCrash = true);

  const ProcessArguments &arguments() const;
  void setArguments(const ProcessArguments &arguments);

  /// \note At least two arguments should be given.
  template <typename Arg0, typename Arg1, typename... Args>
  void setArguments(Arg0 &&arg0, Arg1 &&arg1, Args &&... args) {
    setArguments(system::execution::collect(std::forward<Arg0>(arg0),
                                            std::forward<Arg1>(arg1),
                                            std::forward<Args>(args)...));
  }

  const boost::filesystem::path &currentPath() const;
  void setCurrentPath(const boost::filesystem::path &currentPath);

  const ProcessEnvironment &environment() const;
  void setEnvironment(const ProcessEnvironment &environment);

  std::string environment(const std::string &key) const;
  void setEnvironment(const std::string &key, const std::string &value);
  void unsetEnvironment(const std::string &key);

  const ResourceLimits &resourceLimits() const;
  void setResourceLimits(const ResourceLimits &resourceLimits);

  const system::unistd::access::Id &ownerId() const;
  void setOwnerId(const system::unistd::access::Id &ownerId);

  /*!
   * \brief Name is used in Notifier's events
   *
   * \see notifier::ProcessId
   */
  const std::string &name() const;
  void setName(const std::string &name);

  /*!
   * \return Process::Result previously set
   * by ProcessGroup::poll() or ProcessGroup::wait().
   *
   * \throws ContainerIllegalStateError if process result was not set.
   */
  const Result &result() const;

  /*!
   * \brief Assign descriptor to stream.
   */
  void setStream(int descriptor, const Stream &stream);

  /*!
   * \brief Assign descriptor to non-pipe stream.
   */
  void setNonPipeStream(int descriptor, const NonPipeStream &stream);

  /*!
   * \brief Get assigned stream.
   *
   * \throws ProcessDescriptorOutOfRangeError
   * if no assigned stream exists.
   */
  Stream stream(int descriptor) const;

  /*!
   * \brief Close descriptor.
   */
  void closeStream(int descriptor);

  /*!
   * \brief If descriptor has assigned stream.
   */
  bool hasStream(int descriptor) const;

  /*!
   * \brief Process identifier.
   *
   * Identifier is unique between processes
   * of one process group.
   *
   * \warning User should not rely on value distribution.
   * Such behavior may be changed in the future.
   */
  Id id() const;

 private:
  /*!
   * \warning Constructor is private because
   * class uses own reference-counting mechanism.
   */
  explicit Process(const ProcessGroupPointer &processGroup, Id id);

 private:
  ProcessGroupPointer processGroup_;
  const Id id_;
};

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
