#define BOOST_TEST_MODULE notifier
#include <boost/test/unit_test.hpp>

#include <yandex/contest/invoker/Notifier.hpp>
#include <yandex/contest/invoker/notifier/ObjectConnection.hpp>
#include <yandex/contest/invoker/notifier/QueuedEventWriter.hpp>

#include <yandex/contest/system/unistd/Pipe.hpp>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <unordered_map>

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

    typedef boost::asio::posix::stream_descriptor Connection;

    boost::asio::io_service ioService;
    unistd::Pipe pipe;
    Connection writeEnd;
    yan::ObjectConnection<Connection> oc;
    yac::Notifier notifier;
};

BOOST_FIXTURE_TEST_CASE(Notifier, NotifierFactory)
{
    using yac::Notifier;

    Notifier::Spawn::Event spawnEvent;
    spawnEvent.meta.id = 1;
    spawnEvent.meta.name = "id for spawn";

    Notifier::Termination::Event terminationEvent;
    terminationEvent.meta.id = 2;
    terminationEvent.meta.name = "id for termination";
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
            BOOST_CHECK_EQUAL(event.meta.id, 1);
            BOOST_CHECK_EQUAL(event.meta.name, "id for spawn");
        });
    notifier.onTermination(
        [&](const Notifier::Termination::Event &event)
        {
            termination = true;
            BOOST_CHECK_EQUAL(event.meta.id, 2);
            BOOST_CHECK_EQUAL(event.meta.name, "id for termination");
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
    notifier.start();

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

BOOST_FIXTURE_TEST_CASE(QueuedWriter, NotifierFactory)
{
    boost::mutex boostTestLock;
#define BOOST_TEST_LOCK \
    boost::lock_guard<boost::mutex> boostTestLockGuard(boostTestLock)

    yan::QueuedEventWriter<Connection> writer(
        oc,
        [&](const boost::system::error_code &ec)
        {
            BOOST_TEST_LOCK;
            BOOST_TEST_MESSAGE(ec);
            return true;
        }
    );

    std::unordered_map<std::size_t, int> running;
    boost::mutex runningLock;
    std::size_t runningEvents = 0;
    const auto checkRunning =
        [&](const std::size_t id)
        {
            const auto iter = running.find(id);
            if (iter != running.end() && iter->second == 0)
                running.erase(iter);
            ++runningEvents;
        };
    const auto addRunning =
        [&](const std::size_t id)
        {
            boost::lock_guard<boost::mutex> lk(runningLock);
            {
                //BOOST_TEST_LOCK;
                //BOOST_TEST_MESSAGE("Add " << id);
            }
            ++running[id];
            checkRunning(id);
        };
    const auto delRunning =
        [&](const std::size_t id)
        {
            boost::lock_guard<boost::mutex> lk(runningLock);
            --running[id];
            checkRunning(id);
        };

    notifier.onSpawn(
        [&](const yac::Notifier::Spawn::Event &event)
        {
            addRunning(event.meta.id);
        });
    notifier.onTermination(
        [&](const yac::Notifier::Termination::Event &event)
        {
            delRunning(event.meta.id);
        });
    notifier.onError(
        [&](const yac::Notifier::Error::Event &event)
        {
            BOOST_TEST_LOCK;
            BOOST_CHECK_EQUAL(event.errorCode, boost::asio::error::eof);
        });
    notifier.start();

    boost::thread_group threads;
    for (std::size_t i = 0; i < 10; ++i)
        threads.create_thread(boost::bind(
            &boost::asio::io_service::run,
            &ioService
        ));

    constexpr std::size_t WORKERS = 10;
    constexpr std::size_t STEPS = 10;
    boost::barrier close(2 * WORKERS + 1);

    for (std::size_t i = 0; i < WORKERS; ++i)
        threads.create_thread(
            [&, i]()
            {
                for (std::size_t j = 0; j < STEPS; ++j)
                {
                    yac::Notifier::Spawn::Event event;
                    event.meta.id = i * STEPS + j;
                    writer.write(event);
                }
                {
                    BOOST_TEST_LOCK;
                    BOOST_TEST_MESSAGE("Spawn worker " <<
                                       i << " has finished.");
                }
                close.count_down_and_wait();
            });

    for (std::size_t i = 0; i < WORKERS; ++i)
        threads.create_thread(
            [&, i]()
            {
                for (std::size_t j = 0; j < STEPS; ++j)
                {
                    yac::Notifier::Termination::Event event;
                    event.meta.id = i * STEPS + j;
                    writer.write(event);
                }
                {
                    BOOST_TEST_LOCK;
                    BOOST_TEST_MESSAGE("Termination worker " <<
                                       i << " has finished.");
                }
                close.count_down_and_wait();
            });

    {
        BOOST_TEST_LOCK;
        BOOST_TEST_MESSAGE("Waiting for workers' barrier...");
    }
    close.count_down_and_wait();
    {
        BOOST_TEST_LOCK;
        BOOST_TEST_MESSAGE("Closing writer...");
    }
    writer.close();

    {
        BOOST_TEST_LOCK;
        BOOST_TEST_MESSAGE("Waiting for threads...");
    }
    threads.join_all();
    BOOST_CHECK(running.empty());
    BOOST_CHECK_EQUAL(runningEvents, 2 * STEPS * WORKERS);
}

BOOST_FIXTURE_TEST_CASE(QueuedWriterLog, NotifierFactory)
{
    // check that this constructor compiles
    yan::QueuedEventWriter<Connection> writer(oc);
}

BOOST_AUTO_TEST_SUITE_END() // notifier
