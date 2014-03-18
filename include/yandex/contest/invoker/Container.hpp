#pragma once

#include <yandex/contest/invoker/ContainerConfig.hpp>
#include <yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp>
#include <yandex/contest/invoker/Filesystem.hpp>
#include <yandex/contest/invoker/filesystem/Config.hpp>
#include <yandex/contest/invoker/Forward.hpp>
#include <yandex/contest/invoker/process_group/DefaultSettings.hpp>

#include <yandex/contest/IntrusivePointeeBase.hpp>
#include <yandex/contest/system/lxc/Lxc.hpp>

namespace yandex{namespace contest{namespace invoker
{
    class Container: public IntrusivePointeeBase
    {
    public:
        /*!
         * \brief Create new container using specified config.
         */
        static ContainerPointer create(const ContainerConfig &config);

        /*!
         * \brief Create new container using
         * specified config with patched lxcConfig.
         *
         * \see create()
         */
        static ContainerPointer create(const ContainerConfig &config,
                                       const system::lxc::Config &lxcConfig);

        /*!
         * \brief Create new container using
         * default config with patched lxcConfig.
         *
         * \see ContainerConfig::ContainerConfig()
         */
        static ContainerPointer create(const system::lxc::Config &lxcConfig);

        /*!
         * \brief Create new container using default config.
         *
         * \see ContainerConfig::ContainerConfig()
         */
        static ContainerPointer create();

        /*!
         * \brief Destroy container.
         *
         * Stop all running processes, associated with container.
         */
        ~Container()=default;

        /*!
         * \brief Access to the container's filesystem.
         *
         * Filesystem object lifetime is equivalent to Container
         * object lifetime. It is not recommended to store pointer
         * or reference to Filesystem object if you are not sure
         * that pointer's lifetime is not greater than Container's.
         */
        Filesystem &filesystem();

        /*!
         * \brief Create new process group, associated with container.
         *
         * \see yandex::contest::invoker::ProcessGroup::create()
         */
        ProcessGroupPointer createProcessGroup();

        /*!
         * \brief Default settings for process group.
         *
         * Will be used by every created instance of ProcessGroup.
         *
         * \see ProcessGroupDefaultSettings
         */
        const process_group::DefaultSettings &processGroupDefaultSettings() const;

        /*!
         * \brief Set process group default settings.
         * \see processGroupDefaultSettings()
         */
        void setProcessGroupDefaultSettings(
            const process_group::DefaultSettings &processGroupDefaultSettings);

    private:
        friend class ProcessGroup;

        /*!
         * \brief Execute process group in container.
         *
         * \note This function should not be used directly.
         *
         * \see createProcessGroup()
         */
        detail::execution::AsyncProcessGroup execute(
            const detail::execution::AsyncProcessGroup::Task &task);

        /// \copydoc system::lxc::Lxc::freeze()
        void freeze();

        /// \copydoc system::lxc::Lxc::unfreeze()
        void unfreeze();

        /// \copydoc system::lxc::Lxc::stop()
        void stop();

        /// \copydoc system::lxc::Lxc::state()
        system::lxc::Lxc::State state();

    private:
        /*!
         * \warning Constructor is private because
         * class uses own reference-counting mechanism.
         * Lifetime of Container object is depended on
         * lifetime of ProcessGroup objects.
         *
         * \see create()
         */
        Container(std::unique_ptr<system::lxc::Lxc> &&lxcPtr,
                  const ContainerConfig &config);

    private:
        Filesystem filesystem_;
        const detail::execution::AsyncProcess::Options controlProcessOptions_;
        process_group::DefaultSettings processGroupDefaultSettings_;
        std::unique_ptr<system::lxc::Lxc> lxcPtr_;
    };
}}}
