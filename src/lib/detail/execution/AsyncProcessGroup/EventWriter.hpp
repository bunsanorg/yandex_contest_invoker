#pragma once

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/notifier/Event.hpp>

#include <boost/asio.hpp>

#include <memory>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

class EventWriter : private boost::noncopyable {
 public:
  using EventWriterPointer = std::shared_ptr<EventWriter>;
  using Connection = boost::asio::posix::stream_descriptor;

 public:
  virtual void write(const notifier::Event &event) = 0;
  virtual void close() = 0;

  virtual ~EventWriter() {}

 public:
  static EventWriterPointer instance(NotificationStream::Protocol protocol,
                                     Connection &connection);
};

using EventWriterPointer = EventWriter::EventWriterPointer;

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
