#include <yandex/contest/invoker/lxc/NetworkConfig.hpp>

#include <yandex/contest/invoker/lxc/ConfigHelper.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace lxc {

namespace {
void outputInet(std::ostream &out, const std::string &type,
                const NetworkConfigEntry::Inet &inet) {
  for (const auto &addr : inet.addr) {
    config_helper::output(out, "lxc.network." + type, addr);
  }
  config_helper::optionalOutput(out, "lxc.network." + type + ".gateway",
                                inet.gateway);
}
}  // namespace

std::ostream &operator<<(std::ostream &out, const NetworkConfigEntry &config) {
  config_helper::output(out, "lxc.network.type", config.type);
  config_helper::optionalOutput(out, "lxc.network.flags", config.flags);
  config_helper::optionalOutput(out, "lxc.network.link", config.link);
  config_helper::optionalOutput(out, "lxc.network.mtu", config.mtu);
  config_helper::optionalOutput(out, "lxc.network.name", config.name);
  config_helper::optionalOutput(out, "lxc.network.hwaddr", config.hwaddr);
  outputInet(out, "ipv4", config.ipv4);
  outputInet(out, "ipv6", config.ipv6);
  config_helper::optionalOutput(out, "lxc.network.script.up", config.script.up);
  config_helper::optionalOutput(out, "lxc.network.script.down",
                                config.script.down);
  return out;
}

std::ostream &operator<<(std::ostream &out, const NetworkConfig &config) {
  for (const auto &entry : config) {
    out << entry;
  }
  return out;
}

}  // namespace lxc
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
