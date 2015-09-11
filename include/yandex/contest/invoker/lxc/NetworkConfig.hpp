#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace lxc {

struct NetworkConfigEntry {
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(type);
    ar & BOOST_SERIALIZATION_NVP(flags);
    ar & BOOST_SERIALIZATION_NVP(link);
    ar & BOOST_SERIALIZATION_NVP(mtu);
    ar & BOOST_SERIALIZATION_NVP(name);
    ar & BOOST_SERIALIZATION_NVP(hwaddr);
    ar & BOOST_SERIALIZATION_NVP(ipv4);
    ar & BOOST_SERIALIZATION_NVP(ipv6);
    ar & BOOST_SERIALIZATION_NVP(script);
  }

  std::string type;
  boost::optional<std::string> flags;
  boost::optional<std::string> link;
  boost::optional<std::string> mtu;
  boost::optional<std::string> name;
  boost::optional<std::string> hwaddr;

  struct Inet {
    template <typename Archive>
    void serialize(Archive &ar, const unsigned int) {
      ar & BOOST_SERIALIZATION_NVP(addr);
      ar & BOOST_SERIALIZATION_NVP(gateway);
    }

    std::vector<std::string> addr;
    boost::optional<std::string> gateway;
  };
  Inet ipv4;
  Inet ipv6;

  struct {
    template <typename Archive>
    void serialize(Archive &ar, const unsigned int) {
      ar & BOOST_SERIALIZATION_NVP(up);
      ar & BOOST_SERIALIZATION_NVP(down);
    }

    boost::optional<std::string> up;
    boost::optional<std::string> down;
  } script;
};

using NetworkConfig = std::vector<NetworkConfigEntry>;

std::ostream &operator<<(std::ostream &out, const NetworkConfigEntry &config);
std::ostream &operator<<(std::ostream &out, const NetworkConfig &config);

}  // namespace lxc
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
