#pragma once

/*!
 * \file
 *
 * \brief Useful defines and functions
 * for lxc::Config and lxc::MountConfig implementation.
 *
 * \warning Do not include it in header files.
 */

#include <yandex/contest/invoker/lxc/Error.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <iostream>

namespace yandex{namespace contest{namespace invoker{
    namespace lxc{namespace config_helper
{
    inline std::string toString(const boost::filesystem::path &arg)
    {
        return arg.string();
    }

    template <typename T>
    inline std::string toString(const T &arg)
    {
        return boost::lexical_cast<std::string>(arg);
    }

    template <typename T>
    void output(std::ostream &out, const std::string &key, const T &value)
    {
        if (key.find('=') != std::string::npos)
            BOOST_THROW_EXCEPTION(
                ConfigError() <<
                Error::message("'=' character is prohibited in config key") <<
                ConfigError::key(key));
        const std::string line = key + " = " + toString(value);
        if (line.find('\n') != std::string::npos)
            BOOST_THROW_EXCEPTION(
                ConfigError() <<
                Error::message("'\\n' character is prohibited in config line") <<
                ConfigError::key(key) <<
                ConfigError::line(line));
        out << line << std::endl;
    }

    template <typename T>
    void optionalOutput(
        std::ostream &out,
        const std::string &key,
        const boost::optional<T> &value)
    {
        if (value)
            output(out, key, value.get());
    }

    template <typename T>
    void override_patch(
        boost::optional<T> &this_,
        const boost::optional<T> &patch)
    {
        if (patch)
            this_ = patch;
    }

    template <typename T>
    void recursive_patch(
        boost::optional<T> &this_,
        const boost::optional<T> &patch)
    {
        if (patch)
        {
            if (this_)
                this_->patch(patch.get());
            else
                this_ = patch;
        }
    }
}}}}}

#define BOOST_OPTIONAL_OVERRIDE_PATCH(FIELD) \
    ::yandex::contest::invoker:: \
        lxc::config_helper::override_patch(FIELD, config.FIELD)
#define BOOST_OPTIONAL_RECURSIVE_PATCH(FIELD) \
    ::yandex::contest::invoker:: \
    lxc::config_helper::recursive_patch(FIELD, config.FIELD)
