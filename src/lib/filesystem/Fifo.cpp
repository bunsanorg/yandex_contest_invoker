#include <yandex/contest/invoker/filesystem/Fifo.hpp>

#include <yandex/contest/system/unistd/Operations.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    void Fifo::mknod() const
    {
        system::unistd::mkfifo(path, mode);
        chmod();
    }
}}}}
