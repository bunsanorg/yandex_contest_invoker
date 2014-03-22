#include <yandex/contest/invoker/Notifier.hpp>

#include <yandex/contest/invoker/notifier/ObjectConnection.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/variant/static_visitor.hpp>

namespace yandex{namespace contest{namespace invoker
{
#define YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(LNAME, UNAME) \
    void operator()(const UNAME::Event &event) \
    { \
        ioService_.dispatch(boost::bind( \
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
        public boost::enable_shared_from_this<Impl>,
        public boost::static_visitor<void>,
        private boost::noncopyable
    {
    public:
        Impl(io_service &ioService, const int notifierFd):
            ioService_(ioService),
            notifierFd_(ioService_, notifierFd),
            notifierConnection_(notifierFd_) {}

        void async_start()
        {
            read();
        }

        void close()
        {
            notifierConnection_.close();
        }

        void dispatch(const Event::Event &event)
        {
            boost::apply_visitor(*this, event);
        }

        YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(event, Event)
        YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(error, Error)
        YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(spawn, Spawn)
        YANDEX_CONTEST_NOTIFIER_SIGNAL_IMPL(termination, Termination)

    private:
        void read()
        {
            notifierConnection_.async_read(
                inboundEvent_,
                boost::bind(
                    &Impl::handle_read,
                    this,
                    boost::asio::placeholders::error
                )
            );
        }

        void handle_read(const boost::system::error_code &ec)
        {
            if (ec)
            {
                Error::Event error;
                error.errorCode = ec;
                (*this)(error);
            }
            else
            {
                (*this)(inboundEvent_);
                read();
            }
        }

    private:
        friend class Notifier;

        io_service &ioService_;
        posix::stream_descriptor notifierFd_;
        notifier::ObjectConnection<
            posix::stream_descriptor
        > notifierConnection_;

        Event::Event inboundEvent_;
    };

    Notifier::Notifier(io_service &ioService, const int notifierFd):
        pimpl(new Impl(ioService, notifierFd))
    {
        onEvent(
            Event::Slot(
                &Impl::dispatch,
                pimpl.get(),
                _1
            ).track(pimpl)
        );
    }

    Notifier::~Notifier() { /*~Impl()*/ }

    void Notifier::async_start()
    {
        pimpl->async_start();
    }

    void Notifier::close()
    {
        pimpl->close();
    }

    YANDEX_CONTEST_NOTIFIER_SIGNAL(event, Event)
    YANDEX_CONTEST_NOTIFIER_SIGNAL(error, Error)
    YANDEX_CONTEST_NOTIFIER_SIGNAL(spawn, Spawn)
    YANDEX_CONTEST_NOTIFIER_SIGNAL(termination, Termination)
}}}
