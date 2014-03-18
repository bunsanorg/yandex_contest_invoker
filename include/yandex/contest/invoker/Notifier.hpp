#pragma once

#include <yandex/contest/invoker/notifier/SpawnEvent.hpp>
#include <yandex/contest/invoker/notifier/TerminationEvent.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <boost/signals2/signal.hpp>

#include <memory>

namespace yandex{namespace contest{namespace invoker
{
    class Notifier: private boost::noncopyable
    {
    public:
        typedef boost::signals2::connection Connection;

        template <typename EventType>
        struct EventTypes
        {
            typedef EventType Event;
            typedef boost::signals2::signal<void (const Event &)> Signal;
            typedef typename Signal::slot_type Slot;
            typedef typename Signal::extended_slot_type ExtendedSlot;
        };

        typedef EventTypes<notifier::SpawnEvent> Spawn;
        typedef EventTypes<notifier::TerminationEvent> Termination;

    public:
        Connection onSpawn(const Spawn::Slot &slot);
        Connection onSpawnExtended(const Spawn::ExtendedSlot &slot);

        Connection onTermination(const Termination::Slot &slot);
        Connection onTerminationExtended(const Termination::ExtendedSlot &slot);

    public:
        Notifier(boost::asio::io_service &ioService, const int notifierFd);
        ~Notifier();

        void async_start();
        void stop();

    private:
        class Impl;
        std::shared_ptr<Impl> pimpl;
    };
}}}
