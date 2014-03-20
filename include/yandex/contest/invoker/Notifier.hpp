#pragma once

#include <yandex/contest/invoker/notifier/Event.hpp>
#include <yandex/contest/invoker/notifier/ErrorEvent.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>

namespace yandex{namespace contest{namespace invoker
{
    /*!
     * \note Any event except Error is sent
     * via onEvent() and on{Event}().
     * Error is local event sent on local error.
     */
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

        typedef EventTypes<notifier::Event> Event;
        typedef EventTypes<notifier::ErrorEvent> Error;
        typedef EventTypes<notifier::SpawnEvent> Spawn;
        typedef EventTypes<notifier::TerminationEvent> Termination;

    public:
        Connection onEvent(const Event::Slot &slot);
        Connection onEventExtended(const Event::ExtendedSlot &slot);

        Connection onError(const Error::Slot &slot);
        Connection onErrorExtended(const Error::ExtendedSlot &slot);

        Connection onSpawn(const Spawn::Slot &slot);
        Connection onSpawnExtended(const Spawn::ExtendedSlot &slot);

        Connection onTermination(const Termination::Slot &slot);
        Connection onTerminationExtended(const Termination::ExtendedSlot &slot);

    public:
        Notifier(boost::asio::io_service &ioService, const int notifierFd);
        ~Notifier();

        void async_start();
        void close();

    private:
        class Impl;
        boost::shared_ptr<Impl> pimpl;
    };
}}}
