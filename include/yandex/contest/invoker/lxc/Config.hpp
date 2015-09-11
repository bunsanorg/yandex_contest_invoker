#pragma once

#include <yandex/contest/invoker/lxc/MountConfig.hpp>
#include <yandex/contest/invoker/lxc/NetworkConfig.hpp>
#include <yandex/contest/invoker/lxc/RootfsConfig.hpp>

#include <bunsan/stream_enum.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace yandex {
namespace contest {
namespace invoker {
namespace lxc {

struct Config {
  BUNSAN_INCLASS_STREAM_ENUM_CLASS(Arch, (x86, i686, x86_64, amd64))

  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(arch);
    ar & BOOST_SERIALIZATION_NVP(utsname);
    ar & BOOST_SERIALIZATION_NVP(network);
    // TODO pts
    ar & BOOST_SERIALIZATION_NVP(console);
    ar & BOOST_SERIALIZATION_NVP(tty);
    ar & BOOST_SERIALIZATION_NVP(devttydir);
    ar & BOOST_SERIALIZATION_NVP(mount);
    ar & BOOST_SERIALIZATION_NVP(rootfs);
    ar & BOOST_SERIALIZATION_NVP(pivotdir);
    ar & BOOST_SERIALIZATION_NVP(cgroup);
    ar & boost::serialization::make_nvp("cap.drop", cap_drop);
  }

  boost::optional<Arch> arch;

  /// Hostname.
  boost::optional<std::string> utsname;

  boost::optional<NetworkConfig> network;

  // TODO pts (is not fully implemented by LXC,
  // seems to be useless for now)

  /// Path to a file where the console output will be written.
  boost::optional<boost::filesystem::path> console;

  /// The number of tty available to the container.
  boost::optional<int> tty;

  /// A directory under /dev under which to create
  /// the container console devices.
  boost::optional<boost::filesystem::path> devttydir;

  /// Mount settings.
  boost::optional<MountConfig> mount;

  /// Root filesystem.
  boost::optional<RootfsConfig> rootfs;

  /*!
   * Where to pivot the original root file system under lxc.rootfs,
   * specified relatively to that.
   */
  boost::optional<boost::filesystem::path> pivotdir;

  /*!
   * \brief The control group values to be set.
   *
   * Example (map as json):
   * \code{.json}
   * {"cpuset.cpus": "0", "memory.memsw.limit_in_bytes": "1G"}
   * \endcode
   */
  boost::optional<std::unordered_map<std::string, std::string>> cgroup;

  /*!
   * \brief List of capabilities to be dropped in the container.
   *
   * The format is the lower case of the  capability
   * definition without the "CAP_" prefix,
   * eg. CAP_SYS_MODULE should be specified as sys_module.
   * See capabilities(7).
   */
  boost::optional<std::unordered_set<std::string>> cap_drop;

  void patch(const Config &config);
};

std::ostream &operator<<(std::ostream &out, const Config &config);

}  // namespace lxc
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
