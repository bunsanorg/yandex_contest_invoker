#pragma once

#include <yandex/contest/invoker/notifier/SpawnEvent.hpp>
#include <yandex/contest/invoker/notifier/TerminationEvent.hpp>

#include <boost/serialization/variant.hpp>
#include <boost/variant.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

using Event = boost::variant<SpawnEvent, TerminationEvent>;

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
