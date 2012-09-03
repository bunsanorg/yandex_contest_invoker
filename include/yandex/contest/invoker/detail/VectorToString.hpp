#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <type_traits>

namespace yandex{namespace contest{namespace invoker{namespace detail
{
    /// Converts vector to string in json-like format.
    template <typename T>
    std::string vectorToString(const std::vector<T> &vector)
    {
        std::ostringstream buf;
        buf << "{";
        for (std::size_t i = 0; i < vector.size(); ++i)
        {
            if (i)
                buf << ", ";
            if (!std::is_arithmetic<T>::value)
                buf << '"';
            buf << vector[i];
            if (!std::is_arithmetic<T>::value)
                buf << '"';
        }
        buf << "}";
        return buf.str();
    }
}}}}
