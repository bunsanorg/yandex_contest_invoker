#define BOOST_TEST_MODULE notifier
#include <boost/test/unit_test.hpp>

#include <yandex/contest/invoker/Notifier.hpp>
#include <yandex/contest/invoker/notifier/BlockStream.hpp>
#include <yandex/contest/invoker/notifier/ObjectStream.hpp>

#include <yandex/contest/system/unistd/Pipe.hpp>

#include <boost/asio.hpp>

namespace ya = yandex::contest;
namespace yac = ya::invoker;
namespace yan = yac::notifier;
namespace unistd = ya::system::unistd;

BOOST_AUTO_TEST_SUITE(notifier)

struct SerializationFactory
{
    SerializationFactory():
        socket1(ioService),
        socket2(ioService)
    {
        boost::asio::local::connect_pair(socket1, socket2);
    }

    typedef boost::asio::local::stream_protocol::socket Socket;
    boost::asio::io_service ioService;
    Socket socket1, socket2;
};

BOOST_FIXTURE_TEST_SUITE(serialization, SerializationFactory)

BOOST_AUTO_TEST_CASE(BlockStream)
{
    std::string bs1Data, bs2Data;
    yan::BlockStream<Socket> bs1(socket1), bs2(socket2);
    bs1.async_write("first request",
        [&](const boost::system::error_code &ec)
        {
            BOOST_REQUIRE(!ec);
            bs1.async_read(bs1Data,
                [&](const boost::system::error_code &ec)
                {
                    BOOST_REQUIRE(!ec);
                    BOOST_CHECK_EQUAL(bs1Data, "first response");
                    bs1.async_write("second request",
                        [&](const boost::system::error_code &ec)
                        {
                            BOOST_REQUIRE(!ec);
                            bs1.async_read(bs1Data,
                                [&](const boost::system::error_code &ec)
                                {
                                    BOOST_REQUIRE(!ec);
                                    BOOST_CHECK_EQUAL(bs1Data, "second response");
                                    bs1.close();
                                });
                        });
                });
        });
    bs2.async_read(bs2Data,
        [&](const boost::system::error_code &ec)
        {
            BOOST_REQUIRE(!ec);
            BOOST_CHECK_EQUAL(bs2Data, "first request");
            bs2.async_write("first response",
                [&](const boost::system::error_code &ec)
                {
                    BOOST_REQUIRE(!ec);
                    bs2.async_read(bs2Data,
                        [&](const boost::system::error_code &ec)
                        {
                            BOOST_REQUIRE(!ec);
                            BOOST_CHECK_EQUAL(bs2Data, "second request");
                            bs2.async_write("second response",
                                [&](const boost::system::error_code &ec)
                                {
                                    BOOST_REQUIRE(!ec);
                                    bs2.async_read(bs2Data,
                                        [&](const boost::system::error_code &ec)
                                        {
                                            BOOST_REQUIRE_EQUAL(
                                                ec,
                                                boost::asio::error::eof
                                            );
                                        });
                                });
                        });
                });
        });
    ioService.run();
}

BOOST_AUTO_TEST_CASE(ObjectStream)
{
    int os1Data, os2Data;
    yan::ObjectStream<Socket> os1(socket1), os2(socket2);
    os1.async_write(10,
        [&](const boost::system::error_code &ec)
        {
            BOOST_REQUIRE(!ec);
            os1.async_read(os1Data,
                [&](const boost::system::error_code &ec)
                {
                    BOOST_REQUIRE(!ec);
                    BOOST_CHECK_EQUAL(os1Data, 100);
                    os1.async_write(20,
                        [&](const boost::system::error_code &ec)
                        {
                            BOOST_REQUIRE(!ec);
                            os1.async_read(os1Data,
                                [&](const boost::system::error_code &ec)
                                {
                                    BOOST_REQUIRE(!ec);
                                    BOOST_CHECK_EQUAL(os1Data, 200);
                                    os1.close();
                                });
                        });
                });
        });
    os2.async_read(os2Data,
        [&](const boost::system::error_code &ec)
        {
            BOOST_REQUIRE(!ec);
            BOOST_CHECK_EQUAL(os2Data, 10);
            os2.async_write(100,
                [&](const boost::system::error_code &ec)
                {
                    BOOST_REQUIRE(!ec);
                    os2.async_read(os2Data,
                        [&](const boost::system::error_code &ec)
                        {
                            BOOST_REQUIRE(!ec);
                            BOOST_CHECK_EQUAL(os2Data, 20);
                            os2.async_write(200,
                                [&](const boost::system::error_code &ec)
                                {
                                    BOOST_REQUIRE(!ec);
                                    os2.async_read(os2Data,
                                        [&](const boost::system::error_code &ec)
                                        {
                                            BOOST_REQUIRE_EQUAL(
                                                ec,
                                                boost::asio::error::eof
                                            );
                                        });
                                });
                        });
                });
        });
    ioService.run();
}

BOOST_AUTO_TEST_SUITE_END() // serialization

struct NotifierFactory
{
    NotifierFactory():
        writeEnd(ioService, pipe.releaseWriteEnd().release()),
        oc(writeEnd),
        notifier(ioService, pipe.releaseReadEnd().release()) {}

    boost::asio::io_service ioService;
    unistd::Pipe pipe;
    boost::asio::posix::stream_descriptor writeEnd;
    yan::ObjectStream<boost::asio::posix::stream_descriptor> os;
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
    os.async_write(spawnEvent_,
        [&](const boost::system::error_code &ec)
        {
            BOOST_REQUIRE(!ec);
            os.async_write(terminationEvent_,
                [&](const boost::system::error_code &ec)
                {
                    BOOST_REQUIRE(!ec);
                    os.close();
                });
        });
    ioService.run();
    BOOST_CHECK(error);
    BOOST_CHECK(spawn);
    BOOST_CHECK(termination);
    BOOST_CHECK_EQUAL(eventNumber, 2);
}

BOOST_AUTO_TEST_SUITE_END() // notifier
