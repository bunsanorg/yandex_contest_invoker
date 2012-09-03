#pragma once

#include <boost/filesystem/path.hpp>

namespace yandex{namespace contest{namespace invoker{namespace filesystem
{
    /*!
     * \todo It should be commented I don't know how.
     *
     * This function makes path relative to root
     * (but keeps it inside root).
     */
    boost::filesystem::path keepInRoot(const boost::filesystem::path &path,
                                       const boost::filesystem::path &root);
}}}}
