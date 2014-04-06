#pragma once

#include <bunsan/asio/queued_writer.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <deque>
#include <utility>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    namespace detail
    {
        void logQueuedWriterError(const boost::system::error_code &ec);
    }

    template <typename T, typename Connection>
    class QueuedWriter: public bunsan::asio::queued_writer<T, Connection>
    {
    public:
        typedef bunsan::asio::queued_writer<T, Connection> implementation;
        typedef typename implementation::write_handler WriteHandler;

    public:
        explicit QueuedWriter(Connection &connection):
            QueuedWriter(connection, detail::logQueuedWriterError) {}

        QueuedWriter(
            Connection &connection,
            const WriteHandler &handle_write):
                implementation(connection, handle_write) {}
    };
}}}}
