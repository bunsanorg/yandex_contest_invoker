#pragma once

#include <yandex/contest/invoker/notifier/SpawnEvent.hpp>
#include <yandex/contest/invoker/notifier/TerminationEvent.hpp>

#include <boost/serialization/variant.hpp>
#include <boost/variant.hpp>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    typedef boost::variant<
        SpawnEvent,
        TerminationEvent
    > Event;
}}}}
