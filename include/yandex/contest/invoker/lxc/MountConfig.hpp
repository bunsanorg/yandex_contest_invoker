#pragma once

#include <yandex/contest/system/unistd/MountEntry.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

#include <vector>

namespace yandex {
namespace contest {
namespace invoker {
namespace lxc {

struct MountConfig {
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(fstab);
    ar & BOOST_SERIALIZATION_NVP(entries);
  }

  /// Path to fstab(5) file.
  boost::optional<boost::filesystem::path> fstab;

  /// List of mount entries.
  boost::optional<std::vector<system::unistd::MountEntry>> entries;

  void patch(const MountConfig &config);
};

std::ostream &operator<<(std::ostream &out, const MountConfig &config);

}  // namespace lxc
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
