#include "EventWriter.hpp"

#include "EventWriter/NativeEventWriter.hpp"
#include "EventWriter/PlainTextEventWriter.hpp"

#include <boost/assert.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

EventWriterPointer EventWriter::instance(
    const NotificationStream::Protocol protocol, Connection &connection) {
  EventWriterPointer ptr;
  switch (protocol) {
    case NotificationStream::Protocol::NATIVE:
      ptr.reset(new NativeEventWriter(connection));
      break;
    case NotificationStream::Protocol::PLAIN_TEXT:
      ptr.reset(new PlainTextEventWriter(connection));
      break;
    default:
      BOOST_ASSERT(false);
  }
  return ptr;
}

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
