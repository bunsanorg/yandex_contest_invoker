#include <yandex/contest/invoker/notifier/QueuedWriter.hpp>

#include <yandex/contest/StreamLog.hpp>

namespace yandex{namespace contest{namespace invoker{namespace notifier
{
    namespace detail
    {
        void logQueuedWriterError(const boost::system::error_code &ec)
        {
            if (ec)
                STREAM_ERROR << "Unable to write object: " << ec.message();
        }
    }
}}}}
