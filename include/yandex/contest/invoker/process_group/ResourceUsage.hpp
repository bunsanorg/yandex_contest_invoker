#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace process_group {

struct ResourceUsage {
  template <typename Archive>
  void serialize(Archive & /*ar*/, const unsigned int) {}
};

}  // namespace process_group
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
