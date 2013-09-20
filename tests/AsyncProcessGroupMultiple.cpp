#define BOOST_TEST_MODULE AsyncProcessGroupMultiple
#include <boost/test/unit_test.hpp>

#include "AsyncProcessGroupMultipleFixture.hpp"

#include <bunsan/testing/environment.hpp>
#include <bunsan/testing/filesystem/tempdir.hpp>

#include <boost/lexical_cast.hpp>

#include <chrono>

using namespace bunsan::testing;

BOOST_FIXTURE_TEST_SUITE(multiple, AsyncProcessGroupMultipleFixture)

BOOST_AUTO_TEST_CASE(true_)
{
    p0.executable = "true";
    p1.executable = "true";
    run();
    verifyPGR();
    verifyPRExit(0);
    verifyPRExit(1);
}

BOOST_AUTO_TEST_SUITE(fast_slow)

BOOST_AUTO_TEST_CASE(fast_not_ok)
{
    p0.executable = "false";
    p1.executable = "sleep";
    p1.arguments = {"sleep", sleepTimeStr};
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    // fast
    verifyPRExit(0, 1);
    // slow
    verifyPRSig(1, 9, PR::CompletionStatus::TERMINATED_BY_SYSTEM);
}

BOOST_AUTO_TEST_CASE(fast_not_ok_no_terminate)
{
    p0.executable = "false";
    p0.terminateGroupOnCrash = false;
    p1.executable = "sleep";
    p1.arguments = {"sleep", sleepTimeStr};
    run();
    verifyPGR();
    // fast
    verifyPRExit(0, 1);
    // slow
    verifyPRExit(1);
}

BOOST_AUTO_TEST_CASE(fast_not_ok_no_wait)
{
    p0.executable = "false";
    p0.groupWaitsForTermination = false;
    p1.executable = "sleep";
    p1.arguments = {"sleep", sleepTimeStr};
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    // fast
    verifyPRExit(0, 1);
    // slow
    verifyPRSig(1, 9, PR::CompletionStatus::TERMINATED_BY_SYSTEM);
}

BOOST_AUTO_TEST_CASE(fast_not_ok_no_terminate_no_wait)
{
    p0.executable = "false";
    p0.groupWaitsForTermination = false;
    p0.terminateGroupOnCrash = false;
    p1.executable = "sleep";
    p1.arguments = {"sleep", sleepTimeStr};
    run();
    verifyPGR();
    // fast
    verifyPRExit(0, 1);
    // slow
    verifyPRExit(1);
}

BOOST_AUTO_TEST_SUITE_END() // fast_slow

BOOST_AUTO_TEST_SUITE(pipes)

BOOST_AUTO_TEST_CASE(send_verify)
{
    p0.executable = "echo";
    p0.arguments = {"echo", "arbitrary text"};
    p1.executable = "sh";
    p1.arguments = {"sh", "-ce", "read text; test \"arbitrary text\" = \"$text\""};
    p0.descriptors[1] = pipe(0).writeEnd();
    p1.descriptors[0] = pipe(0).readEnd();
    run();
    verifyPGR();
    verifyPRExit(0);
    verifyPRExit(1);
}

BOOST_AUTO_TEST_CASE(send_back_verify)
{
    p0.executable = "sh";
    p0.arguments = {
        "sh", "-ce",
        "text=\"arbitrary text\"; echo \"$text\";"
        "read x; test \"$text\" = \"$x\""
    };
    p1.executable = "sh";
    p1.arguments = {"sh", "-ce", "read x; echo \"$x\""};
    p0.descriptors[1] = pipe(0).writeEnd();
    p1.descriptors[0] = pipe(0).readEnd();
    p1.descriptors[1] = pipe(1).writeEnd();
    p0.descriptors[0] = pipe(1).readEnd();
    run();
    verifyPGR();
    verifyPRExit(0);
    verifyPRExit(1);
}

BOOST_AUTO_TEST_CASE(chain_send_back_verify)
{
    p(0).executable = "sh";
    p(1).executable = "sh";
    p(2).executable = "sh";
    // p0::1->p1::0 p1::1->p0::1
    p(0).descriptors[1] = pipe(0).writeEnd();
    p(1).descriptors[0] = pipe(0).readEnd();
    p(1).descriptors[1] = pipe(1).writeEnd();
    p(0).descriptors[0] = pipe(1).readEnd();
    // p1::2->p2::0 p2::1->p1::3
    p(1).descriptors[2] = pipe(2).writeEnd();
    p(2).descriptors[0] = pipe(2).readEnd();
    p(2).descriptors[1] = pipe(3).writeEnd();
    p(1).descriptors[3] = pipe(3).readEnd();
    // p0 sends $x to p1
    // p1 reads $x and sends it to p2
    // p2 reads $x and sends it back to p1
    // p1 reads $x and sends it to p0
    // p0 reads $x and verifies that it is original text
    p(0).arguments = {
        "sh", "-ce",
        "text=\"arbitrary text\"; echo \"$text\";"
        "read x; test \"$text\" = \"$x\""
    };
    p(1).arguments = {
        "sh", "-ce",
        "read x; echo \"$x\" >&2;"
        "read y <&3; echo \"$y\""
    };
    p(2).arguments = {"sh", "-ce", "read x; echo \"$x\""};
    run();
    verifyPGR();
    verifyPRExit(0);
    verifyPRExit(1);
    verifyPRExit(2);
}

BOOST_AUTO_TEST_SUITE_END() // pipes

struct BenchmarkFixture: AsyncProcessGroupMultipleFixture
{
    typedef std::chrono::high_resolution_clock Clock;
    typedef Clock::duration Duration;
    typedef Clock::time_point TimePoint;

    static TimePoint now()
    {
        return Clock::now();
    }
};

BOOST_FIXTURE_TEST_SUITE(benchmark, BenchmarkFixture)

struct SendRecvFixture: BenchmarkFixture
{
    const unsigned long count = 1000UL * 1000UL;

    void benchmark(const boost::filesystem::path &client, const boost::filesystem::path &echoServer)
    {
        filesystem::tempdir tmpdir;
        // run benchmark
        p0 = p1 = defaultProcess();
        p0.executable = echoServer;
        p0.descriptors[0] = pipe(0).readEnd();
        p1.descriptors[1] = pipe(0).writeEnd();
        p0.descriptors[1] = pipe(1).writeEnd();
        p1.descriptors[0] = pipe(1).readEnd();
        p0.descriptors[2] = PG::File("echoServer.log", PG::AccessMode::WRITE_ONLY);
        p1.executable = client;
        p1.descriptors[2] = PG::File("client.log", PG::AccessMode::WRITE_ONLY);
        p1.arguments = {"client", boost::lexical_cast<std::string>(count)};
        task.resourceLimits.realTimeLimit = std::chrono::seconds(60);
        p0.resourceLimits.timeLimit = std::chrono::seconds(60);
        p1.resourceLimits = p0.resourceLimits;
        p1.currentPath = p0.currentPath = tmpdir.path;
        const TimePoint beginPoint = now();
        run();
        verifyPGR();
        verifyPRExit(0);
        verifyPRExit(1);
        Duration time = now() - beginPoint;
        // output statistics
        BOOST_TEST_MESSAGE("Execution time: " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(time).count() << " ms.");
        BOOST_TEST_MESSAGE(static_cast<double>(count) /
            std::chrono::duration_cast<std::chrono::seconds>(time).count() << " messages per second.");
        BOOST_TEST_MESSAGE("echoServer: " << 1000 * static_cast<double>(count) /
            std::chrono::duration_cast<std::chrono::milliseconds>(
                pr(0).resourceUsage.userTimeUsage).count() << " messages per cpu second.");
        BOOST_TEST_MESSAGE("client: " << 1000 * static_cast<double>(count) /
            std::chrono::duration_cast<std::chrono::milliseconds>(
                pr(1).resourceUsage.userTimeUsage).count() << " messages per cpu second.");
    }
};

BOOST_FIXTURE_TEST_SUITE(send_recv, SendRecvFixture)

BOOST_AUTO_TEST_CASE(posix)
{
    benchmark(dir::tests::resources::binary() / "benchmark" / "posixClient",
              dir::tests::resources::binary() / "benchmark" / "posixEchoServer");
}

BOOST_AUTO_TEST_CASE(cxx)
{
    benchmark(dir::tests::resources::binary() / "benchmark" / "cxxClient",
              dir::tests::resources::binary() / "benchmark" / "cxxEchoServer");
}

BOOST_AUTO_TEST_SUITE_END() // send_recv

BOOST_AUTO_TEST_SUITE_END() // benchmark

BOOST_AUTO_TEST_SUITE_END() // multiple
