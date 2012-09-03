#pragma once

#include <utility>

#include "yandex/contest/invoker/Error.hpp"

#include "yandex/contest/system/execution/ResultError.hpp"

namespace yandex{namespace contest{namespace invoker
{
    struct ContainerError: virtual Error
    {
        typedef boost::error_info<struct nameTag, std::string> name;
    };

    struct ContainerUtilityError: virtual Error, virtual system::execution::ResultError
    {
        template <typename ... Args>
        explicit ContainerUtilityError(Args &&...args):
            system::execution::ResultError(std::forward<Args>(args)...) {}
    };

    struct ContainerIllegalStateError: virtual ContainerError {};
}}}
