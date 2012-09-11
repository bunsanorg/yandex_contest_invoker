#include "yandex/contest/invoker/filesystem/Device.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include "sys/stat.h"

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    void Device::mknod() const
    {
        mode_t mode_ = mode;
        switch (type)
        {
        case CHAR:
            mode_ |= S_IFCHR;
            break;
        case BLOCK:
            mode_ |= S_IFBLK;
            break;
        }
        system::unistd::mknod(path, mode_, system::unistd::makedev(major, minor));
        chmod();
    }
}}}}
