#pragma once

#include <yandex/contest/invoker/process_group/ResourceUsage.hpp>

#include <bunsan/stream_enum.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{namespace process_group
{
    BUNSAN_STREAM_ENUM_CLASS(State,
    (
        STOPPED,
        STARTING,
        RUNNING,
        STOPPING,
        ABORTING,
        FREEZING,
        FROZEN,
        THAWED
    ))
}}}}
