#pragma once

#include <yandex/contest/invoker/notifier/BlockStream.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>

#include <sstream>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    template <typename Stream>
    class ObjectStream
    {
    public:
        explicit ObjectStream(Stream &stream): blockStream_(stream) {}

        template <typename T, typename Handler>
        void async_write(const T &obj, Handler handler)
        {
            std::ostringstream sout;
            {
                boost::archive::text_oarchive oa(sout);
                oa << obj;
            }
            blockStream_.async_write(sout.str(), handler);
        }

        template <typename T, typename Handler>
        void async_read(T &obj, Handler handler)
        {
            blockStream_.async_read(data_,
                [this, &obj, handler](const boost::system::error_code &ec)
                {
                    handle_read(ec, obj, handler);
                });
        }

        void close()
        {
            blockStream_.close();
        }

    private:
        template <typename T, typename Handler>
        void handle_read(
            const boost::system::error_code &ec,
            T &obj,
            Handler handler)
        {
            if (ec)
            {
                handler(ec);
            }
            else
            {
                std::istringstream sin(data_);
                try
                {
                    boost::archive::text_iarchive ia(sin);
                    ia >> obj;
                }
                catch (std::exception &)
                {
                    boost::system::error_code error(
                        boost::asio::error::invalid_argument);
                    handler(ec);
                    return;
                }
                handler(ec);
            }
        }

    private:
        BlockStream<Stream> blockStream_;
        std::string data_;
    };
}}}}
