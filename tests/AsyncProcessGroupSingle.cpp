#define BOOST_TEST_MODULE AsyncProcessGroupSingle
#include <boost/test/unit_test.hpp>

#include "AsyncProcessGroupSingleFixture.hpp"

#include <csignal>
#include <cerrno>

#include <boost/lexical_cast.hpp>

BOOST_FIXTURE_TEST_SUITE(single, AsyncProcessGroupSingleFixture)

BOOST_AUTO_TEST_CASE(true_)
{
    process.executable = "true";
    run();
    verifyPGR();
    verifyPRExit(0);
}

BOOST_AUTO_TEST_CASE(false_)
{
    process.executable = "false";
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    verifyPRExit(0, 1);
}

BOOST_AUTO_TEST_CASE(abort_)
{
    process.executable = "sh";
    process.arguments = {"sh", "-ce", "kill -9 $$"};
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    verifyPRSig(0);
}

BOOST_AUTO_TEST_CASE(currentPath)
{
    process.executable = "sh";
    process.arguments = {"sh", "-ce", "echo -n \"$PWD\""};
    process.currentPath = "/tmp";
    const TMP output;
    process.descriptors[1] = PG::File(output.path());
    run();
    verifyPGR();
    verifyPRExit(0);
    BOOST_CHECK_EQUAL(readData(output.path()), process.currentPath.string());
}

BOOST_AUTO_TEST_CASE(currentPathNoX)
{
    TempDir tmpdir;
    process.executable = "sh";
    process.arguments = {"sh", "-ce", "echo -n \"$PWD\""};
    process.currentPath = tmpdir.path;
    unistd::chmod(tmpdir.path, 0700);
    unistd::chown(tmpdir.path, unistd::access::Id(0, 0));
    process.ownerId = {1000, 1000}; // arbitrary non-zero
    const TMP output;
    process.descriptors[1] = PG::File(output.path());
    run();
    verifyPGR();
    verifyPRExit(0);
    BOOST_CHECK_EQUAL(process.currentPath.string(), readData(output.path()));
}

BOOST_AUTO_TEST_SUITE(resource_usage)

// TODO

BOOST_AUTO_TEST_SUITE_END() // resource_usage

BOOST_AUTO_TEST_SUITE(resource_limits)

BOOST_AUTO_TEST_CASE(real_time_limit)
{
    process.executable = "sleep";
    process.arguments = {"sleep", decaSleepTime};
    task.resourceLimits.realTimeLimitMillis = sleepTimeMillis;
    run();
    verifyPGR(PGR::CompletionStatus::REAL_TIME_LIMIT_EXCEEDED);
}

BOOST_AUTO_TEST_SUITE(cpu_limit)

BOOST_AUTO_TEST_CASE(busy_beaver)
{
    process.executable = "perl";
    // busy beaver
    process.arguments = {"perl", "-e", "while (true) {}"};
    task.resourceLimits.realTimeLimitMillis = 10 * std::max(cpuLimitResolutionMillis,
                                                            decaSleepTimeMillis);
    task.processes[0].resourceLimits.timeLimitMillis = sleepTimeMillis;
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    verifyPR(0, PR::CompletionStatus::TIME_LIMIT_EXCEEDED);
}

BOOST_AUTO_TEST_CASE(unstable)
{
    process.executable = "perl";
    // busy beaver
    process.arguments = {"perl", "-e", "while (true) {}"};
    task.resourceLimits.realTimeLimitMillis = 10 * cpuLimitResolutionMillis;
    // 1 second (from setrlimit(3)::RLIMIT_CPU resolution)
    process.resourceLimits.timeLimitMillis = cpuLimitResolutionMillis;
    // this happens sometimes
    // so we need statically reliable test
    for (std::size_t i = 0; i < 5; ++i)
    {
        run();
        verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
        verifyPR(0, PR::CompletionStatus::TIME_LIMIT_EXCEEDED);
    }
}

BOOST_AUTO_TEST_CASE(exec)
{
    process.executable = testsResourcesBinaryDir / "exec";
    TMP tmpfile;
    process.descriptors[2] = PG::File(tmpfile.path());
    process.resourceLimits.timeLimitMillis = 10 * std::max(decaSleepTimeMillis,
                                                           cpuLimitResolutionMillis);
    task.resourceLimits.realTimeLimitMillis = 10 * process.resourceLimits.timeLimitMillis;
    // single execution is OK
    std::uint64_t singleCpuUsage;
    {
        process.arguments = {process.executable.string(), "-1"};
        run();
        verifyPGR();
        verifyPRExit(0);
        BOOST_REQUIRE_EQUAL(result.processGroupResult.completionStatus, PGR::CompletionStatus::OK);
        singleCpuUsage = pr(0).resourceUsage.timeUsageMillis;
        BOOST_REQUIRE_GT(singleCpuUsage, 0);
        BOOST_TEST_MESSAGE("Single cpu usage: " << singleCpuUsage);
    }
    // but a lot of exec are not
    {
        process.arguments = {process.executable.string(), "0"};
        run();
        const std::string output = readData(tmpfile.path());
        const std::size_t execNumber2x = std::count(output.begin(), output.end(), '\n');
        BOOST_REQUIRE_GT(execNumber2x, 2 * 4);
        BOOST_TEST_MESSAGE("Exec number: " << static_cast<double>(execNumber2x) / 2);
        std::uint64_t multipleCpuUsage = pr(0).resourceUsage.timeUsageMillis;
        BOOST_TEST_MESSAGE("Multiple cpu usage: " << multipleCpuUsage);
        BOOST_REQUIRE_GT(multipleCpuUsage, 0);
        std::uint64_t estimatedMultipleCpuUsage = (singleCpuUsage * execNumber2x) / 2;
        BOOST_TEST_MESSAGE("Estimated multiple cpu usage: " << estimatedMultipleCpuUsage);
        double multipleCpuUsageError = std::fabs(static_cast<double>(multipleCpuUsage) -
                                                 estimatedMultipleCpuUsage) / multipleCpuUsage;
        BOOST_TEST_MESSAGE("Estimated multiple cpu usage error: " << multipleCpuUsageError);
        BOOST_CHECK_SMALL(multipleCpuUsageError, 0.5);
        verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
        verifyPR(0, PR::CompletionStatus::TIME_LIMIT_EXCEEDED);
    }
}

BOOST_AUTO_TEST_SUITE_END() // cpu_limit

BOOST_AUTO_TEST_SUITE(number_of_processes)

BOOST_AUTO_TEST_CASE(forker)
{
    TMP tmpfile;
    process.executable = testsResourcesSourceDir / "forker.py";
    process.ownerId = uniqueOwnerId;
    process.descriptors[1] = PG::File(tmpfile.path());
    process.resourceLimits.numberOfProcesses = 10;
    task.resourceLimits.realTimeLimitMillis = decaSleepTimeMillis;
    run();
    verifyPGR();
    verifyPRExit(0);
    const std::uint64_t output = boost::lexical_cast<std::uint64_t>(readData(tmpfile.path()));
    // interpreter + number of created processes
    BOOST_CHECK_EQUAL(output + 1, process.resourceLimits.numberOfProcesses);
}

BOOST_AUTO_TEST_CASE(true_1)
{
    process.executable = "true";
    process.ownerId = uniqueOwnerId;
    process.resourceLimits.numberOfProcesses = 1;
    run();
    verifyPGR();
    verifyPRExit(0);
}

BOOST_AUTO_TEST_CASE(exec_1)
{
    process.executable = testsResourcesSourceDir / "exec_1.py";
    process.ownerId = uniqueOwnerId;
    process.resourceLimits.numberOfProcesses = 1;
    process.arguments = {process.executable.string(), "0"};
    task.resourceLimits.realTimeLimitMillis = decaSleepTimeMillis; // python is slowpoke...
    run();
    verifyPGR();
    verifyPRExit(0);
}

BOOST_AUTO_TEST_SUITE_END() // number_of_processes

BOOST_AUTO_TEST_SUITE_END() // resource_limits

BOOST_AUTO_TEST_CASE(no_wait)
{
    process.executable = "sleep";
    // it will be visible if process is not killed instantly
    process.arguments = {"sleep", decaSleepTime};
    process.groupWaitsForTermination = false;
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    verifyPRSig(0);
}

BOOST_AUTO_TEST_SUITE(uid_gid)

BOOST_AUTO_TEST_CASE(default_)
{
    const TMP output;
    process.executable = "sh";
    process.arguments = {"sh", "-ce", "echo -n `id -ru`:`id -rg`:`id -u`:`id -g`"};
    process.descriptors[1] = PG::File(output.path());
    run();
    verifyPGR();
    verifyPRExit(0);
    BOOST_CHECK_EQUAL(readData(output.path()), "0:0:0:0");
}

BOOST_AUTO_TEST_CASE(drop)
{
    const TMP output;
    process.executable = "sh";
    process.arguments = {"sh", "-ce", "echo -n `id -ru`:`id -rg`:`id -u`:`id -g`"};
    process.ownerId.uid = 1234;
    process.ownerId.gid = 1235;
    process.descriptors[1] = PG::File(output.path());
    run();
    verifyPGR();
    verifyPRExit(0);
    BOOST_CHECK_EQUAL(readData(output.path()), "1234:1235:1234:1235");
}

BOOST_AUTO_TEST_SUITE_END() // uid_gid

BOOST_AUTO_TEST_CASE(open_files)
{
    const std::string data = "arbitrary string";
    const TMP input(data), output;
    process.executable = "cat";
    process.descriptors[0] = PG::File(input.path());
    process.descriptors[1] = PG::File(output.path());
    run();
    verifyPGR();
    verifyPRExit(0);
    BOOST_CHECK_EQUAL(readData(output.path()), data);
}

BOOST_AUTO_TEST_CASE(start_failed)
{
    process.executable = boost::filesystem::temp_directory_path() /
        boost::filesystem::unique_path();
    BOOST_REQUIRE(!boost::filesystem::exists(process.executable));
    run();
    verifyPGR(PGR::CompletionStatus::ABNORMAL_EXIT);
    BOOST_CHECK_EQUAL(pr(0).completionStatus, PR::CompletionStatus::START_FAILED);
    // note: other fields of pr(0) have unspecified values
}

BOOST_AUTO_TEST_SUITE(fd_alias)

BOOST_AUTO_TEST_CASE(stderr_to_stdout)
{
    const TMP tmpfile;
    process.executable = testsResourcesSourceDir / "stderr_to_stdout.py";
    process.descriptors[1] = PG::File(tmpfile.path());
    process.descriptors[2] = PG::FDAlias(1);
    run();
    verifyPGR();
    verifyPRExit(0);
    BOOST_CHECK_EQUAL(readData(tmpfile.path()), "stdoutstderr");
}

// Note: tests validate the message of exception.
// It would be better to validate exception
// type, but it is not forwarded.

namespace
{
    bool checkUnresolvedFDAliasError(const ya::ResultError &e)
    {
        return std::string(e.what()).find("UnresolvedFDAliasError") != std::string::npos;
    }
}

BOOST_AUTO_TEST_CASE(UnresolvedFDAliasErrorTest)
{
    process.executable = "true";
    process.descriptors[2] = PG::FDAlias(3);
    BOOST_CHECK_EXCEPTION(run(), ya::ResultError, checkUnresolvedFDAliasError);
}

namespace
{
    bool checkInvalidTargetFDAliasError(const ya::ResultError &e)
    {
        return std::string(e.what()).find("InvalidTargetFDAliasError") != std::string::npos;
    }
}

BOOST_AUTO_TEST_CASE(InvalidTargetFDAliasErrorTest)
{
    process.executable = "true";
    process.descriptors[2] = PG::FDAlias(3);
    process.descriptors[3] = PG::FDAlias(1);
    BOOST_CHECK_EXCEPTION(run(), ya::ResultError, checkInvalidTargetFDAliasError);
}

BOOST_AUTO_TEST_SUITE_END() // fd_alias

BOOST_AUTO_TEST_SUITE(security)

BOOST_AUTO_TEST_CASE(lost_child)
{
    process.executable = testsResourcesSourceDir / "lost_child.py";
    TMP tmpfile;
    process.descriptors[1] = PG::File(tmpfile.path());
    run();
    verifyPGR();// FIXME which???
    verifyPRExit(0);
    const pid_t pid = boost::lexical_cast<pid_t>(readData(tmpfile.path()));
    const int ret = kill(pid, 0);
    const int errno_ = errno;
    BOOST_CHECK_LT(ret, 0);
    BOOST_CHECK_EQUAL(errno_, ESRCH);
}

BOOST_AUTO_TEST_SUITE_END() // security

BOOST_AUTO_TEST_SUITE_END() // single
