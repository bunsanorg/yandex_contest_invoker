#pragma once

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/notifier/QueuedEventWriter.hpp>
#include <yandex/contest/invoker/process/Result.hpp>

#include <boost/signals2/signal.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    class Notifier: private boost::noncopyable
    {
    public:
        typedef boost::signals2::signal<
            void (const ProcessMeta &)
        > SpawnSignal;

        typedef boost::signals2::signal<
            void (const ProcessMeta &, const process::Result &)
        > TerminationSignal;

        typedef boost::signals2::signal<void ()> CloseSignal;

        struct Signals
        {
            SpawnSignal spawn;
            TerminationSignal termination;
            CloseSignal close;
        };

    public:
        Notifier(boost::asio::io_service &ioService, const int fd);

        void spawn(const ProcessMeta &processMeta);

        void termination(
            const ProcessMeta &processMeta,
            const process::Result &result);

        void close();

    private:
        typedef boost::asio::posix::stream_descriptor Connection;

        Connection fd_;
        notifier::ObjectConnection<Connection> connection_;
        notifier::QueuedEventWriter<Connection> writer_;
    };
}}}}}}
