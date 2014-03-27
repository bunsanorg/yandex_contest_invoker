#include "Notifier.hpp"

namespace yandex{namespace contest{namespace invoker{
    namespace detail{namespace execution{namespace async_process_group_detail
{
    Notifier::Notifier(boost::asio::io_service &ioService, const int fd):
        fd_(ioService, fd),
        connection_(fd_),
        writer_(connection_) {}

    void Notifier::spawn(const ProcessMeta &processMeta)
    {
        notifier::SpawnEvent event;
        event.meta = processMeta;
        writer_.write(event);
    }

    void Notifier::termination(
        const ProcessMeta &processMeta,
        const process::Result &result)
    {
        notifier::TerminationEvent event;
        event.meta = processMeta;
        event.result = result;
        writer_.write(event);
    }

    void Notifier::close()
    {
        writer_.close();
    }
}}}}}}
