#pragma once

#include "../EventWriter.hpp"

#include <yandex/contest/invoker/notifier/QueuedEventWriter.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

class NativeEventWriter : public EventWriter {
 public:
  NativeEventWriter(Connection &connection);

  void write(const notifier::Event &event) override;

  void close() override;

 private:
  notifier::ObjectConnection<Connection> connection_;
  notifier::QueuedEventWriter<Connection> writer_;
};

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
