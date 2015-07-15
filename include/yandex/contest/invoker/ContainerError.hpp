#pragma once

#include <yandex/contest/invoker/Error.hpp>

#include <yandex/contest/system/execution/ResultError.hpp>

namespace yandex {
namespace contest {
namespace invoker {

struct ContainerError : virtual Error {
  using name = boost::error_info<struct nameTag, std::string>;
};

struct ContainerUtilityError : system::execution::ResultError, virtual Error {
  using system::execution::ResultError::ResultError;
};

struct ContainerIllegalStateError : virtual ContainerError {};

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
