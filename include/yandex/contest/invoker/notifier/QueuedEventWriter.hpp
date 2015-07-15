#pragma once

#include <yandex/contest/invoker/notifier/Event.hpp>
#include <yandex/contest/invoker/notifier/ObjectConnection.hpp>
#include <yandex/contest/invoker/notifier/QueuedWriter.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

template <typename Connection>
using QueuedEventWriter = QueuedWriter<Event, ObjectConnection<Connection>>;

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
