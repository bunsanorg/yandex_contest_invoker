#include <yandex/contest/invoker/filesystem/Directory.hpp>

#include <yandex/contest/system/unistd/Operations.hpp>

#include <boost/filesystem/operations.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace filesystem {

void Directory::mknod() const {
  if (!boost::filesystem::is_directory(path))
    system::unistd::mkdir(path.c_str(), mode);
  chmod();
}

}  // namespace filesystem
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
