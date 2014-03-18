#include <yandex/contest/invoker/Notifier.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace yandex{namespace contest{namespace invoker
{
#define YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(LNAME, UNAME) \
    void LNAME(const UNAME::Event &event) \
    { \
        ioService_.post(boost::bind( \
            &Impl::dispatch##UNAME, \
            shared_from_this(), \
            event \
        )); \
    } \
    void dispatch##UNAME(const UNAME::Event &event) \
    { \
        LNAME##_(event); \
    } \
    UNAME::Signal LNAME##_;

#define YANDEX_CONTEST_NOTIFIER_SIGNAL(LNAME, UNAME) \
    Notifier::Connection Notifier::on##UNAME( \
        const UNAME::Slot &slot) \
    { \
        return pimpl->LNAME##_.connect(slot); \
    } \
    Notifier::Connection Notifier::on##UNAME##Extended( \
        const UNAME::ExtendedSlot &slot) \
    { \
        return pimpl->LNAME##_.connect_extended(slot); \
    }

    using namespace boost::asio;

    class Notifier::Impl:
        public std::enable_shared_from_this<Impl>,
        private boost::noncopyable
    {
    public:
        Impl(io_service &ioService, const int notifierFd):
            ioService_(ioService),
            notifierFd_(ioService_, notifierFd) {}

        void async_start()
        {
#warning "Not implemented"
        }

        void stop()
        {
#warning "Not implemented"
        }

    private:
        YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(spawn, Spawn)
        YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(termination, Termination)

    private:
        friend class Notifier;

        io_service &ioService_;
        posix::stream_descriptor notifierFd_;
    };

    Notifier::Notifier(io_service &ioService, const int notifierFd):
        pimpl(new Impl(ioService, notifierFd)) {}

    Notifier::~Notifier() { /*~Impl()*/ }

    void Notifier::async_start()
    {
        pimpl->async_start();
    }

    void Notifier::stop()
    {
        pimpl->stop();
    }

    YANDEX_CONTEST_NOTIFIER_SIGNAL(spawn, Spawn)
    YANDEX_CONTEST_NOTIFIER_SIGNAL(termination, Termination)
}}}
