#include <yandex/contest/invoker/filesystem/SymLink.hpp>

#include <yandex/contest/system/unistd/Operations.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    void SymLink::mknod() const
    {
        system::unistd::symlink(value, path);
    }
}}}}
