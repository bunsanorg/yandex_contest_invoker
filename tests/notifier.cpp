#define BOOST_TEST_MODULE notifier
#include <boost/test/unit_test.hpp>

#include <yandex/contest/invoker/Notifier.hpp>
#include <yandex/contest/invoker/notifier/ObjectConnection.hpp>

#include <yandex/contest/system/unistd/Pipe.hpp>

#include <boost/asio.hpp>

namespace ya = yandex::contest;
namespace yac = ya::invoker;
namespace yan = yac::notifier;
namespace unistd = ya::system::unistd;

BOOST_AUTO_TEST_SUITE(notifier)

struct NotifierFactory
{
    NotifierFactory():
        writeEnd(ioService, pipe.releaseWriteEnd().release()),
        oc(writeEnd),
        notifier(ioService, pipe.releaseReadEnd().release()) {}

    boost::asio::io_service ioService;
    unistd::Pipe pipe;
    boost::asio::posix::stream_descriptor writeEnd;
    yan::ObjectConnection<boost::asio::posix::stream_descriptor> oc;
    yac::Notifier notifier;
};

BOOST_FIXTURE_TEST_CASE(Notifier, NotifierFactory)
{
    using yac::Notifier;

    Notifier::Spawn::Event spawnEvent;
    spawnEvent.processId.id = 1;
    spawnEvent.processId.name = "id for spawn";

    Notifier::Termination::Event terminationEvent;
    terminationEvent.processId.id = 2;
    terminationEvent.processId.name = "id for termination";
    terminationEvent.result.completionStatus =
        yac::process::Result::CompletionStatus::TIME_LIMIT_EXCEEDED;
    terminationEvent.result.exitStatus = boost::none;
    terminationEvent.result.termSig = 9;
    terminationEvent.result.resourceUsage.timeUsage = std::chrono::seconds(2);
    terminationEvent.result.resourceUsage.memoryUsageBytes = 1024;

    bool error = false;
    bool spawn = false;
    bool termination = false;
    std::size_t eventNumber = 0;
    notifier.onEvent(
        [&](const Notifier::Event::Event &event)
        {
            BOOST_CHECK(!boost::get<Notifier::Error::Event *>(&event));
            ++eventNumber;
        });
    notifier.onError(
        [&](const Notifier::Error::Event &event)
        {
            error = true;
            BOOST_CHECK_EQUAL(event.errorCode, boost::asio::error::eof);
        });
    notifier.onSpawn(
        [&](const Notifier::Spawn::Event &event)
        {
            spawn = true;
            BOOST_CHECK_EQUAL(event.processId.id, 1);
            BOOST_CHECK_EQUAL(event.processId.name, "id for spawn");
        });
    notifier.onTermination(
        [&](const Notifier::Termination::Event &event)
        {
            termination = true;
            BOOST_CHECK_EQUAL(event.processId.id, 2);
            BOOST_CHECK_EQUAL(event.processId.name, "id for termination");
            BOOST_CHECK_EQUAL(
                event.result.completionStatus,
                yac::process::Result::CompletionStatus::TIME_LIMIT_EXCEEDED);
            BOOST_CHECK(!event.result.exitStatus);
            BOOST_CHECK_EQUAL(event.result.termSig, 9);
            BOOST_CHECK( // note: chrono does not support operator<<()
                event.result.resourceUsage.timeUsage ==
                std::chrono::seconds(2)
            );
            BOOST_CHECK_EQUAL(
                event.result.resourceUsage.memoryUsageBytes,
                1024
            );
        });
    notifier.async_start();

    const Notifier::Event::Event spawnEvent_(spawnEvent);
    const Notifier::Event::Event terminationEvent_(terminationEvent);
    oc.async_write(spawnEvent_,
        [&](const boost::system::error_code &ec)
        {
            BOOST_REQUIRE(!ec);
            oc.async_write(terminationEvent_,
                [&](const boost::system::error_code &ec)
                {
                    BOOST_REQUIRE(!ec);
                    oc.close();
                });
        });
    ioService.run();
    BOOST_CHECK(error);
    BOOST_CHECK(spawn);
    BOOST_CHECK(termination);
    BOOST_CHECK_EQUAL(eventNumber, 2);
}

BOOST_AUTO_TEST_SUITE_END() // notifier
