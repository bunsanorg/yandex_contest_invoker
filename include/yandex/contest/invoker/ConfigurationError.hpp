#pragma once

#include <yandex/contest/invoker/Error.hpp>

namespace yandex {
namespace contest {
namespace invoker {

struct ConfigurationError : virtual Error {};

struct InvalidEnumValueError : virtual ConfigurationError {
  using value = boost::error_info<struct valueTag, std::string>;
};

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
