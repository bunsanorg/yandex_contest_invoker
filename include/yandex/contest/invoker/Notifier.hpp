#pragma once

#include <yandex/contest/invoker/notifier/ErrorEvent.hpp>
#include <yandex/contest/invoker/notifier/Event.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>

namespace yandex {
namespace contest {
namespace invoker {

/*!
 * \note Any event except Error is sent
 * via onEvent() and on{Event}().
 * Error is local event sent on local error.
 */
class Notifier : private boost::noncopyable {
 public:
  using Connection = boost::signals2::connection;

  template <typename EventType>
  struct EventTypes {
    using Event = EventType;
    using Signal = boost::signals2::signal<void(const Event &)>;
    using Slot = typename Signal::slot_type;
    using ExtendedSlot = typename Signal::extended_slot_type;
  };

  using Event = EventTypes<notifier::Event>;
  using Error = EventTypes<notifier::ErrorEvent>;
  using Spawn = EventTypes<notifier::SpawnEvent>;
  using Termination = EventTypes<notifier::TerminationEvent>;

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
  Notifier(boost::asio::io_service &ioService, int notifierFd);
  ~Notifier();

  void start();
  void close();

 private:
  class Impl;
  boost::shared_ptr<Impl> pimpl;
};

}  // namespace invoker
}  // namespace contest
}  // namespace yandex
