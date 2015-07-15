#pragma once

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/process/Result.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

struct TerminationEvent {
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(meta);
    ar & BOOST_SERIALIZATION_NVP(result);
  }

  ProcessMeta meta;
  process::Result result;
};

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
