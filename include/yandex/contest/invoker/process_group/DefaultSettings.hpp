#pragma once

#include <yandex/contest/invoker/Forward.hpp>
#include <yandex/contest/invoker/process/DefaultSettings.hpp>
#include <yandex/contest/invoker/process_group/ResourceLimits.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace process_group {

struct DefaultSettings {
  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(resourceLimits);
    ar & BOOST_SERIALIZATION_NVP(processDefaultSettings);
  }

  ResourceLimits resourceLimits;
  process::DefaultSettings processDefaultSettings;

  void setUpProcessGroup(const ProcessGroupPointer &processGroup) const;
};

}  // namespace process_group
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
