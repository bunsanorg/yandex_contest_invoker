#include "Notifier.hpp"

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

Notifier::Notifier(boost::asio::io_service &ioService, const int fd,
                   const NotificationStream::Protocol protocol)
    : fd_(ioService, fd), writer_(EventWriter::instance(protocol, fd_)) {}

void Notifier::spawn(const ProcessMeta &processMeta) {
  notifier::SpawnEvent event;
  event.meta = processMeta;
  writer_->write(event);
}

void Notifier::termination(const ProcessMeta &processMeta,
                           const process::Result &result) {
  notifier::TerminationEvent event;
  event.meta = processMeta;
  event.result = result;
  writer_->write(event);
}

void Notifier::close() { writer_->close(); }

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
