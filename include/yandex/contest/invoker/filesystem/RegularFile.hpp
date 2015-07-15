#pragma once

#include <yandex/contest/invoker/filesystem/Error.hpp>
#include <yandex/contest/invoker/filesystem/File.hpp>

#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace filesystem {

struct RegularFile : File {
  struct InvalidSourceError : virtual Error {
    using source = boost::error_info<struct sourceTag, boost::filesystem::path>;
  };
  struct SourceDoesNotExistsError : virtual InvalidSourceError {};
  struct SourceIsNotRegularFileError : virtual InvalidSourceError {};

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int) {
    ar & static_cast<File &>(*this);
    ar & BOOST_SERIALIZATION_NVP(source);
  }

  boost::optional<boost::filesystem::path> source;

 protected:
  virtual void mknod() const;
};

}  // namespace filesystem
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
