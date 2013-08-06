#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>

#include "ProcessGroupStarter.hpp"

namespace yandex{namespace contest{namespace invoker{namespace detail{namespace execution
{
    AsyncProcessGroup::Result AsyncProcessGroup::execute(const Task &task)
    {
        async_process_group_detail::ProcessGroupStarter starter(task);
        starter.executionLoop();
        return starter.result();
    }
}}}}}
