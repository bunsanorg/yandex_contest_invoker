#pragma once

#include <boost/serialization/access.hpp>
#include <bunsan/serialization/chrono.hpp>
#include <boost/serialization/nvp.hpp>

#include <chrono>

namespace yandex {
namespace contest {
namespace invoker {
namespace process_group {

struct ResourceLimits {
  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    using boost::serialization::make_nvp;
    ar & make_nvp("realTimeLimitMillis", realTimeLimit);
  }

  std::chrono::milliseconds realTimeLimit = std::chrono::seconds(10);
};

}  // namespace process_group
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
