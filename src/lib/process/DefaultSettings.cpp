#include <yandex/contest/invoker/process/DefaultSettings.hpp>

#include <yandex/contest/invoker/Process.hpp>

#include <boost/assert.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace process {

void DefaultSettings::setUpProcess(const ProcessPointer &process) const {
  BOOST_ASSERT(process);
  process->setResourceLimits(resourceLimits);
  process->setEnvironment(environment);
  process->setCurrentPath(currentPath);
  process->setOwnerId(ownerId);
  for (const auto &fdStream : descriptors)
    process->setNonPipeStream(fdStream.first, fdStream.second);
}

}  // namespace process
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
