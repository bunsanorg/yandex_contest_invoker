#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <deque>
#include <utility>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    namespace detail
    {
        bool logQueuedWriterError(const boost::system::error_code &ec);
    }

    template <typename T, typename Connection>
    class QueuedWriter
    {
    public:
        /// \return false if error occurred
        typedef boost::function<
            bool (const boost::system::error_code &)
        > WriteHandler;

    public:
        explicit QueuedWriter(Connection &connection):
            QueuedWriter(connection, detail::logQueuedWriterError) {}

        QueuedWriter(
            Connection &connection,
            const WriteHandler &handle_write):
                connection_(connection),
                strand_(connection_.get_io_service()),
                handle_write_(handle_write) {}

        void write(T object)
        {
            strand_.post(boost::bind(
                &QueuedWriter<T, Connection>::push,
                this,
                std::move(object)
            ));
        }

        void close()
        {
            strand_.post(boost::bind(
                &QueuedWriter<T, Connection>::finish,
                this
            ));
        }

    private:
        void push(T object)
        {
            queue_.push_back(std::move(object));
            BOOST_ASSERT(!queue_.empty());
            if (queue_.size() == 1)
            { // queue was empty before, writer is required
                spawn_writer();
            }
        }

        void finish()
        {
            if (last_)
                return;

            last_ = true;
            if (queue_.empty())
                connection_.close();
        }

        void spawn_writer()
        {
            // note: std::deque never invalidates
            // external references on push/pop.
            const T &object = queue_.front();
            connection_.async_write(object, strand_.wrap(boost::bind(
                &QueuedWriter<T, Connection>::handle_write,
                this,
                boost::asio::placeholders::error
            )));
        }

        void handle_write(const boost::system::error_code &ec)
        {
            if (ec)
            {
                // some error codes are not actually errors
                if (ec == boost::asio::error::broken_pipe)
                    return;
                handle_write_(ec);
            }

            queue_.pop_front();

            if (queue_.empty())
            {
                if (last_)
                    connection_.close();
            }
            else
            {
                spawn_writer();
            }
        }

    private:
        Connection connection_;
        boost::asio::io_service::strand strand_;
        bool last_ = false;
        std::deque<T> queue_;
        WriteHandler handle_write_;
    };
}}}}
