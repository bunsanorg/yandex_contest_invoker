#pragma once

/*!
 * \file
 *
 * \brief Common typedefs for yandex::contest::invoker::ProcessGroup
 * and yandex::contest::invoker::Process.
 */

#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>
#include <yandex/contest/invoker/process/Result.hpp>

namespace yandex {
namespace contest {
namespace invoker {

using ProcessMeta = detail::execution::AsyncProcessGroup::ProcessMeta;
using ProcessTask = detail::execution::AsyncProcessGroup::Process;
using Stream = detail::execution::AsyncProcessGroup::Stream;
using NonPipeStream = detail::execution::AsyncProcessGroup::NonPipeStream;
using Pipe = detail::execution::AsyncProcessGroup::Pipe;
using AccessMode = detail::execution::AsyncProcessGroup::AccessMode;
using File = detail::execution::AsyncProcessGroup::File;
using FdAlias = detail::execution::AsyncProcessGroup::FdAlias;
using NotificationStream =
    detail::execution::AsyncProcessGroup::NotificationStream;

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
