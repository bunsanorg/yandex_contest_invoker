#include <yandex/contest/invoker/notifier/QueuedWriter.hpp>

#include <yandex/contest/detail/LogHelper.hpp>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    namespace detail
    {
        bool logQueuedWriterError(const boost::system::error_code &ec)
        {
            STREAM_ERROR << "Unable to write object: " << ec.message();
            return false;
        }
    }
}}}}
