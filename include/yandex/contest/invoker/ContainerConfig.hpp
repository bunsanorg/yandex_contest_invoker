#pragma once

#include <yandex/contest/invoker/ControlProcessConfig.hpp>
#include <yandex/contest/invoker/filesystem/Config.hpp>
#include <yandex/contest/invoker/process_group/DefaultSettings.hpp>

#include <yandex/contest/system/lxc/Config.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

#include <iostream>

namespace yandex{namespace contest{namespace invoker
{
    struct ContainerConfig
    {
        /// Default configuration.
        ContainerConfig();

        ContainerConfig(const ContainerConfig &)=default;
        ContainerConfig &operator=(const ContainerConfig &)=default;

        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            using boost::serialization::make_nvp;

            ar & BOOST_SERIALIZATION_NVP(containersDir);
            ar & make_nvp("lxc", lxcConfig);
            ar & BOOST_SERIALIZATION_NVP(processGroupDefaultSettings);
            ar & make_nvp("controlProcess", controlProcessConfig);
            ar & make_nvp("filesystem", filesystemConfig);
        }

        boost::filesystem::path containersDir;
        system::lxc::Config lxcConfig;
        process_group::DefaultSettings processGroupDefaultSettings;
        ControlProcessConfig controlProcessConfig;
        filesystem::Config filesystemConfig;

        /*!
         * \brief Load ContainerConfig from file specified by INVOKER_CONFIG
         * environment variable. If variable is not specified default instance
         * is returned.
         *
         * \note Primary usage: unit tests, other non-interactive execution
         * where it is hard to pass argument.
         */
        static ContainerConfig fromEnvironment();
    };

    std::istream &operator>>(std::istream &in, ContainerConfig &config);
    std::ostream &operator<<(std::ostream &out, const ContainerConfig &config);
}}}
