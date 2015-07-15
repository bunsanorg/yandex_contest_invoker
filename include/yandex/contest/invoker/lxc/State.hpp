#pragma once

#include <bunsan/stream_enum.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace lxc {
namespace lxc_detail {

/*!
 * \brief LXC states.
 *
 * \warning Get in sync with ::lxc_state_t.
 */
BUNSAN_STREAM_ENUM_CLASS(State, (
  STOPPED,
  STARTING,
  RUNNING,
  STOPPING,
  ABORTING,
  FREEZING,
  FROZEN,
  THAWED
))

}  // namespace lxc_detail
}  // namespace lxc
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
