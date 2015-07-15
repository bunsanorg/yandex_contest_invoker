#include <yandex/contest/invoker/filesystem/CreateFile.hpp>

#include <yandex/contest/invoker/filesystem/Operations.hpp>

#include <boost/variant/static_visitor.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace filesystem {

namespace {
struct CreateVisitor : boost::static_visitor<void> {
  template <typename T>
  void operator()(T file) const {
    file.path = keepInRoot(file.path, root);
    file.create();
  }

  boost::filesystem::path root;
};
}  // namespace

void CreateFile::create() const { create("/"); }

void CreateFile::create(const boost::filesystem::path &root) const {
  CreateVisitor visitor;
  visitor.root = root;
  boost::apply_visitor(visitor, file_);
}

}  // namespace filesystem
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
