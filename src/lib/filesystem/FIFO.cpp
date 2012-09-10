#include "yandex/contest/invoker/filesystem/FIFO.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    void FIFO::mknod() const
    {
        system::unistd::mkfifo(path, mode);
        chmod();
    }
}}}}
