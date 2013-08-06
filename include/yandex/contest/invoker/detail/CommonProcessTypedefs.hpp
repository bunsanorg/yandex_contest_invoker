#pragma once

/*!
 * \file
 *
 * \brief Common typedefs for yandex::contest::invoker::ProcessGroup
 * and yandex::contest::invoker::Process.
 */

#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>
#include <yandex/contest/invoker/process/Result.hpp>

namespace yandex{namespace contest{namespace invoker
{
    typedef detail::execution::AsyncProcessGroup::Process ProcessTask;
    typedef detail::execution::AsyncProcessGroup::Stream Stream;
    typedef detail::execution::AsyncProcessGroup::NonPipeStream NonPipeStream;
    typedef detail::execution::AsyncProcessGroup::Pipe Pipe;
    typedef detail::execution::AsyncProcessGroup::AccessMode AccessMode;
    typedef detail::execution::AsyncProcessGroup::File File;
    typedef detail::execution::AsyncProcessGroup::FDAlias FDAlias;
}}}
