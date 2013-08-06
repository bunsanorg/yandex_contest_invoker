#pragma once

#include <yandex/contest/system/execution/AsyncProcess.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker
{
    /*!
     * \brief Config for control process
     *
     * Class is intended to be used by system administrator
     * or package maintainer. These settings are system
     * and usually should not be changed after installation.
     *
     * \see ContainerConfig
     * \see system::execution::AsyncProcessGroup
     * \see system::execution::AsyncProcessGroup::execute()
     */
    struct ControlProcessConfig
    {
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(executable);
        }

        boost::filesystem::path executable;

        explicit operator system::execution::AsyncProcess::Options() const;
    };
}}}
