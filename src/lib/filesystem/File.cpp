#include <yandex/contest/invoker/filesystem/File.hpp>

#include <yandex/contest/system/unistd/Operations.hpp>

#include <yandex/contest/SystemError.hpp>

#include <boost/filesystem/operations.hpp>

#include <sys/stat.h>
#include <unistd.h>

namespace yandex {
namespace contest {
namespace invoker {
namespace filesystem {

namespace {
void createDirectories(const boost::filesystem::path &path) {
  if (!boost::filesystem::exists(path)) {
    createDirectories(path.parent_path());
    system::unistd::mkdir(path, 0777);
    system::unistd::chown(path, {0, 0});
    system::unistd::chmod(path, 0777);
  }
}
}  // namespace

void File::create() const {
  const boost::filesystem::path abs = boost::filesystem::absolute(path);
  const boost::filesystem::path dir = abs.parent_path();
  createDirectories(dir);
  mknod();
  chown();
}

void File::chmod() const { system::unistd::chmod(path, mode); }

void File::chown() const {
  // if we created a SymLink
  // we want to change it's owner
  system::unistd::lchown(path, ownerId);
}

}  // namespace filesystem
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
