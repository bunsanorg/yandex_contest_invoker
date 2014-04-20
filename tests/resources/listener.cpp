#include <yandex/contest/invoker/Notifier.hpp>

#include <boost/asio.hpp>
#include <boost/assert.hpp>

#include <iostream>

#include <fcntl.h>

using namespace yandex::contest::invoker;

int main()
{
    boost::asio::io_service ioService;
    Notifier notifier(ioService, STDIN_FILENO);

    std::size_t spawn_count = 0;
    notifier.onSpawn(
        [&](const Notifier::Spawn::Event &event)
        {
            BOOST_ASSERT(
                event.meta.name == "worker" ||
                event.meta.name == "listener"
            );
            ++spawn_count;
        });

    std::size_t termination_count = 0;
    notifier.onTermination(
        [&](const Notifier::Termination::Event &event)
        {
            BOOST_ASSERT(event.meta.name == "worker");
            BOOST_ASSERT(event.result.completionStatus ==
                         process::Result::CompletionStatus::OK);
            ioService.stop();
            ++termination_count;
        });

    notifier.onError(
        [](const Notifier::Error::Event &event)
        {
            std::cerr << event.errorCode.message() << std::endl;
            std::abort();
        });

    notifier.start();
    ioService.run();
    BOOST_ASSERT(spawn_count == 2);
    BOOST_ASSERT(termination_count == 1);
}
