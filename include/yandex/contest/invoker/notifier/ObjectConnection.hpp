#pragma once

#include <bunsan/asio/text_object_connection.hpp>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    template <typename Connection>
    using ObjectConnection = bunsan::asio::text_object_connection<Connection>;
}}}}
