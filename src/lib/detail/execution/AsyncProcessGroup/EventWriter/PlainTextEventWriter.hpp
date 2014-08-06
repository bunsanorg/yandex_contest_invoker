#pragma once

#include "../EventWriter.hpp"

#include <yandex/contest/invoker/notifier/QueuedWriter.hpp>

#include <bunsan/asio/line_connection.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    class PlainTextEventWriter: public EventWriter
    {
    public:
        PlainTextEventWriter(Connection &connection);

        void write(const notifier::Event &event) override;

        void close() override;

    private:
        bunsan::asio::line_connection<Connection> connection_;
        notifier::QueuedWriter<
            std::string,
            bunsan::asio::line_connection<Connection>
        > writer_;
    };
}}}}}}
