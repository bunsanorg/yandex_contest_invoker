#pragma once

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

struct SpawnEvent {
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(meta);
  }

  ProcessMeta meta;
};

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
