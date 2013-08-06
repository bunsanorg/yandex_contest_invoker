#pragma once

#include <yandex/contest/invoker/Error.hpp>

#include <yandex/contest/system/execution/ResultError.hpp>

#include <bunsan/forward_constructor.hpp>

namespace yandex{namespace contest{namespace invoker
{
    struct ContainerError: virtual Error
    {
        typedef boost::error_info<struct nameTag, std::string> name;
    };

    struct ContainerUtilityError: virtual Error, virtual system::execution::ResultError
    {
        BUNSAN_INHERIT_EXPLICIT_CONSTRUCTOR(ContainerUtilityError, system::execution::ResultError)
    };

    struct ContainerIllegalStateError: virtual ContainerError {};
}}}
