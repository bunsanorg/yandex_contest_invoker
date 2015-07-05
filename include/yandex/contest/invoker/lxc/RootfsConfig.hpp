#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace lxc
{
    struct RootfsConfig
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(fsname);
            ar & BOOST_SERIALIZATION_NVP(mount);
        }

        /// Image file, a directory or a block device for root file system.
        boost::optional<boost::filesystem::path> fsname;

        /// Where to recursively bind lxc.rootfs before pivoting.
        boost::optional<boost::filesystem::path> mount;

        void patch(const RootfsConfig &config);
    };

    std::ostream &operator<<(std::ostream &out, const RootfsConfig &config);
}}}}
