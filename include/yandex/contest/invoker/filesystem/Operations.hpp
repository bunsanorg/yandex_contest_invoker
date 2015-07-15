#pragma once

#include <boost/filesystem/path.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace filesystem {

/*!
 * \todo It should be commented I don't know how.
 *
 * This function makes path relative to root
 * (but keeps it inside root).
 */
boost::filesystem::path keepInRoot(const boost::filesystem::path &path,
                                   const boost::filesystem::path &root);

/// This function makes strips root prefix from path or throws .
boost::filesystem::path containerPath(const boost::filesystem::path &path,
                                      const boost::filesystem::path &root);

}  // namespace filesystem
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
