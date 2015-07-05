#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/intrusive_ptr.hpp>

struct lxc_container;

void intrusive_ptr_add_ref(::lxc_container *) noexcept;
void intrusive_ptr_release(::lxc_container *) noexcept;

namespace yandex{namespace contest{namespace invoker{namespace lxc{
    namespace api
{
    typedef boost::intrusive_ptr<::lxc_container> container_ptr;

    container_ptr container_new(const std::string &name);

    container_ptr container_new(
        const std::string &name,
        const boost::filesystem::path &configPath);
}}}}}
