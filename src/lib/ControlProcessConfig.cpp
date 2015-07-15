#include <yandex/contest/invoker/ControlProcessConfig.hpp>

namespace yandex {
namespace contest {
namespace invoker {

ControlProcessConfig::operator system::execution::AsyncProcess::Options()
    const {
  system::execution::AsyncProcess::Options opts;
  opts.executable = executable;
  return opts;
}

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
