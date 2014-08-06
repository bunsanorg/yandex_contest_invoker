#pragma once

#include <yandex/contest/invoker/detail/CommonProcessTypedefs.hpp>
#include <yandex/contest/invoker/notifier/Event.hpp>

#include <boost/asio.hpp>

#include <memory>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    class EventWriter: private boost::noncopyable
    {
    public:
        typedef std::shared_ptr<EventWriter> EventWriterPointer;
        typedef boost::asio::posix::stream_descriptor Connection;

    public:
        virtual void write(const notifier::Event &event)=0;
        virtual void close()=0;

        virtual ~EventWriter() {}

    public:
        static EventWriterPointer instance(
            const NotificationStream::Protocol protocol,
            Connection &connection);
    };
    typedef EventWriter::EventWriterPointer EventWriterPointer;
}}}}}}
