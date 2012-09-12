#define BOOST_TEST_MODULE AsyncProcessGroupMultiple
#include <boost/test/unit_test.hpp>

#include "AsyncProcessGroupMultipleFixture.hpp"

#include <chrono>

#include <boost/lexical_cast.hpp>

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
    p1.arguments = {"sleep", sleepTime};
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    // fast
    verifyPRExit(0, 1);
    // slow
    verifyPRSig(1);
}

BOOST_AUTO_TEST_CASE(fast_not_ok_no_terminate)
{
    p0.executable = "false";
    p0.terminateGroupOnCrash = false;
    p1.executable = "sleep";
    p1.arguments = {"sleep", sleepTime};
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
    p1.arguments = {"sleep", sleepTime};
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    // fast
    verifyPRExit(0, 1);
    // slow
    verifyPRSig(1);
}

BOOST_AUTO_TEST_CASE(fast_not_ok_no_terminate_no_wait)
{
    p0.executable = "false";
    p0.groupWaitsForTermination = false;
    p0.terminateGroupOnCrash = false;
    p1.executable = "sleep";
    p1.arguments = {"sleep", sleepTime};
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

    TimePoint now()
    {
        return Clock::now();
    }
};

BOOST_FIXTURE_TEST_SUITE(benchmark, BenchmarkFixture)

BOOST_AUTO_TEST_SUITE(send_recv)

template <typename Config>
struct SendRecvFixture: BenchmarkFixture
{
    Config *this_ = static_cast<Config *>(this);

    void benchmark()
    {
        TempDir tmpdir;
        // write sources
        writeData(tmpdir.path / this_->clientSourceName, this_->clientSource);
        writeData(tmpdir.path / this_->echoServerSourceName, this_->echoServerSource);
        // compile
        p0.executable = this_->compilerExecutable;
        p0.currentPath = tmpdir.path;
        p0.arguments = this_->compilerArguments;
        p1 = p0;
        p0.arguments.push_back("-o");
        p0.arguments.push_back("client");
        p0.arguments.push_back(this_->clientSourceName);
        p1.arguments.push_back("-o");
        p1.arguments.push_back("echoServer");
        p1.arguments.push_back(this_->echoServerSourceName);
        run();
        verifyPGR();
        verifyPRExit(0);
        verifyPRExit(1);
        BOOST_REQUIRE_EQUAL(result.processGroupResult.completionStatus, PGR::CompletionStatus::OK);
        // run benchmark
        p0 = p1 = defaultProcess();
        p0.executable = tmpdir.path / "echoServer";
        p0.descriptors[0] = pipe(0).readEnd();
        p1.descriptors[1] = pipe(0).writeEnd();
        p0.descriptors[1] = pipe(1).writeEnd();
        p1.descriptors[0] = pipe(1).readEnd();
        p0.descriptors[2] = PG::File("echoServer.log", PG::AccessMode::WRITE_ONLY);
        p1.executable = tmpdir.path / "client";
        p1.descriptors[2] = PG::File("client.log", PG::AccessMode::WRITE_ONLY);
        p1.arguments = {"client", boost::lexical_cast<std::string>(this_->count)};
        task.resourceLimits.realTimeLimitMillis = 60 * 1000;
        p0.resourceLimits.timeLimitMillis = 60 * 1000;
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
        BOOST_TEST_MESSAGE(static_cast<double>(this_->count) /
            std::chrono::duration_cast<std::chrono::seconds>(time).count() << " messages per second.");
        BOOST_TEST_MESSAGE("echoServer: " << 1000 * static_cast<double>(this_->count) /
            pr(0).resourceUsage.timeUsageMillis << " messages per cpu second.");
        BOOST_TEST_MESSAGE("client: " << 1000 * static_cast<double>(this_->count) /
            pr(1).resourceUsage.timeUsageMillis << " messages per cpu second.");
    }
};

struct SendRecvCFixture
{
    const unsigned long count = 1000UL * 1000UL;

    const std::string bufferSource = R"EOF(
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BUFSIZE 20

char srcBuf[BUFSIZE];

char buf[BUFSIZE];

void clearAll()
{
    memset(buf, 0, BUFSIZE);
    // make invalid
    buf[BUFSIZE - 1] = 1;
}

void fillAll()
{
    unsigned char c = 0;
    for (unsigned i = 0; i + 1 < BUFSIZE; ++i)
        c ^= (srcBuf[i] = buf[i] = 'a' + (rand() % 50));
    srcBuf[BUFSIZE - 1] = buf[BUFSIZE - 1] = c;
}

void fastValidateAll()
{
    unsigned char c = 0;
    for (unsigned i = 0; i < BUFSIZE; ++i)
    {
        if (i + 1 < BUFSIZE)
            assert(buf[i] != 0);
        c ^= buf[i];
    }
    assert(c == 0);
}

void validateAll()
{
    for (unsigned i = 0; i < BUFSIZE; ++i)
        if (srcBuf[i] != buf[i])
        {
            fprintf(stderr, "srcBuf[%u] != buf[%u]: %d != %d\n", i, i, (int)srcBuf[i], (int)buf[i]);
            abort();
        }
}
    )EOF";

    const std::string echoServerCSource = R"EOF(
int main()
{
    clearAll();
    while (readAll())
    {
        fastValidateAll();
        writeAll();
        clearAll();
    }
    return 0;
}
    )EOF";

    const std::string clientCSource = R"EOF(
int main(int argc, char *argv[])
{
    const unsigned long count = atol(argv[1]);
    for (unsigned long i = 0; i < count; ++i)
    {
        fillAll();
        writeAll();
        clearAll();
        readAll();
        validateAll();
    }
    return 0;
}
    )EOF";
};

struct SendRecvPosixFixture: SendRecvFixture<SendRecvPosixFixture>, SendRecvCFixture
{
    const std::string compilerExecutable = "gcc";
    const std::vector<std::string> compilerArguments = {"gcc", "-O2", "-std=c99"};
    const std::string clientSourceName = "client.c";
    const std::string echoServerSourceName = "echoServer.c";

    const std::string commonSource = bufferSource + R"EOF(
#include <unistd.h>

int errRet(int code, const char *cmd)
{
    if (code < 0)
    {
        perror(cmd);
        abort();
    }
    return code;
}

#define ERR(X) errRet(X, #X)

ssize_t readAll()
{
    ssize_t readBytes, readBytesInternal;
    readBytes = ERR(read(STDIN_FILENO, buf, BUFSIZE));
    if (!readBytes)
        return 0;
    while ((readBytes < BUFSIZE) && (readBytesInternal =
           ERR(read(STDIN_FILENO, buf + readBytes, BUFSIZE - readBytes))))
    {
        readBytes += readBytesInternal;
    }
    assert(readBytes == BUFSIZE);
    return readBytes;
}

ssize_t writeAll()
{
    ssize_t writeBytes, writeBytesInternal;
    writeBytes = ERR(write(STDOUT_FILENO, buf, BUFSIZE));
    while ((writeBytes < BUFSIZE) && (writeBytesInternal =
           ERR(write(STDOUT_FILENO, buf + writeBytes, BUFSIZE - writeBytes))))
    {
        writeBytes += writeBytesInternal;
    }
    assert(writeBytes == BUFSIZE);
    return writeBytes;
}

    )EOF";

    const std::string clientSource = commonSource + clientCSource;
    const std::string echoServerSource = commonSource + echoServerCSource;
};

BOOST_FIXTURE_TEST_CASE(posix, SendRecvPosixFixture)
{
    benchmark();
}

struct SendRecvCxxFixture: SendRecvFixture<SendRecvCxxFixture>, SendRecvCFixture
{
    const std::string compilerExecutable = "g++";
    const std::vector<std::string> compilerArguments = {"g++", "-O2"};
    const std::string echoServerSourceName = "echoServer.cpp";
    const std::string clientSourceName = "client.cpp";

    const std::string commonSource = bufferSource + R"EOF(
#include <iostream>

const bool ios_hook = std::ios::sync_with_stdio(false);

bool writeAll()
{
    std::cout.write(buf, BUFSIZE);
    std::cout.flush();
    assert(std::cout);
    return true;
}

bool readAll()
{
    std::cin.read(buf, BUFSIZE);
    if (std::cin.gcount())
    {
        assert(std::cin.gcount() == BUFSIZE);
        assert(std::cin);
        return true;
    }
    else
    {
        return false;
    }
}
    )EOF";

    const std::string clientSource = commonSource + clientCSource;
    const std::string echoServerSource = commonSource + echoServerCSource;
};

BOOST_FIXTURE_TEST_CASE(cxx, SendRecvCxxFixture)
{
    benchmark();
}

BOOST_AUTO_TEST_SUITE_END() // send_recv

BOOST_AUTO_TEST_SUITE_END() // benchmark

BOOST_AUTO_TEST_SUITE_END() // multiple
