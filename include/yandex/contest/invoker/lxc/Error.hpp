#pragma once

#include <yandex/contest/invoker/Error.hpp>
#include <yandex/contest/invoker/lxc/State.hpp>

#include <yandex/contest/system/execution/ResultError.hpp>

#include <bunsan/forward_constructor.hpp>

#include <string>

namespace yandex {
namespace contest {
namespace invoker {
namespace lxc {

struct Error : virtual invoker::Error {
  using name = boost::error_info<struct nameTag, std::string>;
};

struct IllegalStateError : virtual Error {
  using state = boost::error_info<struct stateTag, lxc_detail::State>;
};

struct UtilityError : virtual Error, virtual system::execution::ResultError {
  using system::execution::ResultError::ResultError;
};

struct ConfigError : virtual Error {
  using key = boost::error_info<struct keyTag, std::string>;
  using line = boost::error_info<struct lineTag, std::string>;
};

struct ApiError : virtual Error {};
struct ApiContainerNewError : virtual ApiError {
  using configPath =
      boost::error_info<struct configPathTag, boost::filesystem::path>;
};
struct UnableToCreateContainerError : virtual ApiContainerNewError {};

}  // namespace lxc
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
