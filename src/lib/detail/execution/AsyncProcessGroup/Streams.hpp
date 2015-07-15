#pragma once

#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>

#include <yandex/contest/system/unistd/Descriptor.hpp>
#include <yandex/contest/system/unistd/Pipe.hpp>

#include <boost/variant/static_visitor.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace detail {
namespace execution {
namespace async_process_group_detail {

struct Streams : boost::static_visitor<int> {
  Streams(std::vector<system::unistd::Pipe> &pipes,
          std::vector<system::unistd::Descriptor> &allocatedFds,
          const boost::filesystem::path &currentPath,
          std::unordered_map<int, int> &descriptors)
      : pipes_(&pipes),
        allocatedFds_(&allocatedFds),
        currentPath_(currentPath),
        descriptors_(&descriptors) {}

  int operator()(const AsyncProcessGroup::File &file) const;

  int operator()(const AsyncProcessGroup::Pipe::End &pipeEnd) const;

  int operator()(const AsyncProcessGroup::FdAlias &fdAlias) const;

  bool isAlias(const AsyncProcessGroup::Stream &stream) const;

  int getFd(const AsyncProcessGroup::Stream &stream) const;

 private:
  std::vector<system::unistd::Pipe> *const pipes_;

  /// Descriptors, opened by this object.
  std::vector<system::unistd::Descriptor> *const allocatedFds_;

  /// For File Streams.
  const boost::filesystem::path currentPath_;

  /// For FdAlias Streams.
  std::unordered_map<int, int> *const descriptors_;
};

}  // namespace async_process_group_detail
}  // namespace execution
}  // namespace detail
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
