#pragma once

#include <bunsan/asio/queued_writer.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <deque>
#include <utility>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

namespace detail {
void logQueuedWriterError(const boost::system::error_code &ec);
}  // namespace detail

template <typename T, typename Connection>
class QueuedWriter : public bunsan::asio::queued_writer<T, Connection> {
 public:
  using implementation = bunsan::asio::queued_writer<T, Connection>;
  using WriteHandler = typename implementation::write_handler;

 public:
  explicit QueuedWriter(Connection &connection)
      : QueuedWriter(connection, detail::logQueuedWriterError) {}

  QueuedWriter(Connection &connection, const WriteHandler &handle_write)
      : implementation(connection, handle_write) {}
};

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
