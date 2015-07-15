#pragma once

#include "EventWriter.hpp"

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/process/Result.hpp>

#include <boost/signals2/signal.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

class Notifier : private boost::noncopyable {
 public:
  using SpawnSignal = boost::signals2::signal<void(const ProcessMeta &)>;
  using TerminationSignal = boost::signals2::signal<void(
      const ProcessMeta &, const process::Result &)>;
  using CloseSignal = boost::signals2::signal<void()>;

  struct Signals {
    SpawnSignal spawn;
    TerminationSignal termination;
    CloseSignal close;
  };

 public:
  Notifier(boost::asio::io_service &ioService, int fd,
           NotificationStream::Protocol protocol);

  void spawn(const ProcessMeta &processMeta);

  void termination(const ProcessMeta &processMeta,
                   const process::Result &result);

  void close();

 private:
  using Connection = EventWriter::Connection;

  Connection fd_;
  EventWriterPointer writer_;
};

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
