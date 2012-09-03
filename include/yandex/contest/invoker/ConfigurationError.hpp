#pragma once

#include "yandex/contest/invoker/Error.hpp"

namespace yandex{namespace contest{namespace invoker
{
    struct ConfigurationError: virtual Error {};

    struct InvalidEnumValueError: virtual ConfigurationError
    {
        typedef boost::error_info<struct valueTag, std::string> value;
    };
}}}
