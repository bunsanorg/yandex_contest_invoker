#include "NativeEventWriter.hpp"

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

NativeEventWriter::NativeEventWriter(Connection &connection)
    : connection_(connection), writer_(connection_) {}

void NativeEventWriter::write(const notifier::Event &event) {
  writer_.write(event);
}

void NativeEventWriter::close() { writer_.close(); }

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
