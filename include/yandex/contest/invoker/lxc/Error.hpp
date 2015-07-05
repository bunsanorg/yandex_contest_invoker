#pragma once

#include <yandex/contest/invoker/Error.hpp>
#include <yandex/contest/invoker/lxc/State.hpp>

#include <yandex/contest/system/execution/ResultError.hpp>

#include <bunsan/forward_constructor.hpp>

#include <string>

namespace yandex{namespace contest{namespace invoker{namespace lxc
{
    struct Error: virtual invoker::Error
    {
        typedef boost::error_info<struct nameTag, std::string> name;
    };

    struct IllegalStateError: virtual Error
    {
        typedef boost::error_info<struct stateTag, lxc_detail::State> state;
    };

    struct UtilityError:
        virtual Error,
        virtual system::execution::ResultError
    {
        BUNSAN_INHERIT_EXPLICIT_CONSTRUCTOR(UtilityError, system::execution::ResultError)
    };

    struct ConfigError: virtual Error
    {
        typedef boost::error_info<struct keyTag, std::string> key;
        typedef boost::error_info<struct lineTag, std::string> line;
    };

    struct ApiError: virtual Error {};
    struct ApiContainerNewError: virtual ApiError
    {
        typedef boost::error_info<
            struct configPathTag,
            boost::filesystem::path
        > configPath;
    };
    struct UnableToCreateContainerError: virtual ApiContainerNewError {};
}}}}
