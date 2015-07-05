#include <yandex/contest/invoker/lxc/LxcApi.hpp>

#include <yandex/contest/invoker/lxc/Error.hpp>

#include <lxc/lxccontainer.h>

#include <boost/assert.hpp>

void intrusive_ptr_add_ref(::lxc_container *container) noexcept
{
    BOOST_VERIFY(::lxc_container_get(container));
}

void intrusive_ptr_release(::lxc_container *container) noexcept
{
    BOOST_VERIFY(::lxc_container_put(container) >= 0);
}

namespace yandex{namespace contest{namespace invoker{namespace lxc{
    namespace api
{
    static container_ptr container_new(
        const char *const name,
        const char *const configPath)
    {
        BOOST_ASSERT(name);

        container_ptr ptr(
            ::lxc_container_new(name, configPath),
            false
        );
        if (!ptr)
            BOOST_THROW_EXCEPTION(
                UnableToCreateContainerError() <<
                UnableToCreateContainerError::name(name));
        return ptr;
    }

    container_ptr container_new(const std::string &name)
    {
        return container_new(name.c_str(), nullptr);
    }

    container_ptr container_new(
        const std::string &name,
        const boost::filesystem::path &configPath)
    {
        try
        {
            return container_new(name.c_str(), configPath.string().c_str());
        }
        catch (boost::exception &e)
        {
            e << UnableToCreateContainerError::configPath(configPath);
            throw;
        }
    }
}}}}}
