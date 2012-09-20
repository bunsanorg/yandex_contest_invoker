#pragma once

#include "yandex/contest/tests/BoostExecTest.hpp"
#include "yandex/contest/tests/Environment.hpp"
#include "yandex/contest/tests/Utils.hpp"

#include "yandex/contest/Tempfile.hpp"

#include "yandex/contest/invoker/detail/execution/AsyncProcessGroup.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include <sstream>
#include <iterator>
#include <algorithm>

#include <boost/test/test_tools.hpp>

namespace ya = yandex::contest::invoker::detail::execution;

namespace unistd = yandex::contest::system::unistd;

struct AsyncProcessGroupFixture
{
    typedef ya::AsyncProcessGroup PG;
    typedef yandex::contest::invoker::process_group::Result PGR;
    typedef yandex::contest::invoker::process::Result PR;
    typedef yandex::contest::Tempfile TMP;

    explicit AsyncProcessGroupFixture(const std::size_t size)
    {
        BOOST_REQUIRE_EQUAL(unistd::getuid(), 0);
        task.processes.resize(size, defaultProcess());
    }

    static PG::Process defaultProcess()
    {
        PG::Process pr;
        pr.environment["PATH"] = "/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin";
        pr.environment["HOME"] = "/tmp";
        pr.environment["LANG"] = "C";
        pr.environment["LC_ALL"] = "C";
        pr.descriptors[0] = PG::File("/dev/null");
        pr.descriptors[1] = PG::File("/dev/null");
        pr.descriptors[2] = PG::File("/dev/null");
        return pr;
    }

    void run()
    {
        BOOST_TEST_CHECKPOINT(BOOST_CURRENT_FUNCTION);
        ya::AsyncProcess::Options cfg;
        cfg.executable = "yandex_contest_invoker_ctl";
        PG pg(cfg, task);
        result = pg.wait();
        BOOST_CHECK_EQUAL(pr().size(), task.processes.size());
    }

    const PGR &pgr() const
    {
        return result.processGroupResult;
    }

    const PR &pr(const std::size_t i) const
    {
        BOOST_REQUIRE_LT(i, result.processResults.size());
        return result.processResults[i];
    }

    void verifyPGR(const PGR::CompletionStatus completionStatus=PGR::CompletionStatus::OK) const
    {
        BOOST_CHECK_EQUAL(result.processGroupResult.completionStatus, completionStatus);
    }

    void verifyPR(const std::size_t i,
                  const PR::CompletionStatus completionStatus=PR::CompletionStatus::OK) const
    {
        BOOST_CHECK_EQUAL(pr(i).completionStatus, completionStatus);
    }

    void verifyPRExit(const std::size_t i, const int exitStatus=0) const
    {
        if (exitStatus)
            verifyPR(i, PR::CompletionStatus::ABNORMAL_EXIT);
        else
            verifyPR(i, PR::CompletionStatus::OK);
        BOOST_CHECK(pr(i).exitStatus);
        if (pr(i).exitStatus)
            BOOST_CHECK_EQUAL(pr(i).exitStatus.get(), exitStatus);
        BOOST_CHECK(!pr(i).termSig);
    }

    void verifyPRSig(
        const std::size_t i, const int termSig=9,
        const PR::CompletionStatus completionStatus=PR::CompletionStatus::ABNORMAL_EXIT) const
    {
        verifyPR(i, completionStatus);
        BOOST_CHECK(!pr(i).exitStatus);
        BOOST_CHECK(pr(i).termSig);
        if (pr(i).termSig)
            BOOST_CHECK_EQUAL(pr(i).termSig.get(), termSig);
    }

    PG::Pipe pipe(const std::size_t id)
    {
        BOOST_TEST_CHECKPOINT(BOOST_CURRENT_FUNCTION);
        task.pipesNumber = std::max(task.pipesNumber, id + 1);
        return PG::Pipe(id);
    }

    const std::vector<PR> &pr() const
    {
        return result.processResults;
    }

    AsyncProcessGroupFixture(): AsyncProcessGroupFixture(0) {}

    ~AsyncProcessGroupFixture() {}

    PG::Task task;
    PG::Result result;

    const char *const sleepTime = "0.2";
    const std::uint64_t sleepTimeMillis = 200;
    const char *const decaSleepTime = "2";
    const std::uint64_t decaSleepTimeMillis = 2000;
    const std::uint64_t cpuLimitResolutionMillis = 1000;

    // unique ids, no other processes should use these
    const unistd::access::Id uniqueOwnerId{12345, 12345};
};
