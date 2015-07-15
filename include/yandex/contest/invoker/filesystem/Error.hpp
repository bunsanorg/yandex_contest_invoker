#pragma once

#include <yandex/contest/invoker/Error.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace filesystem {

struct Error : virtual invoker::Error {};
struct NonContainerPathError : virtual Error {};
struct InvalidContainerRootError : virtual Error {};

}  // namespace filesystem
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
