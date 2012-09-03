#define BOOST_TEST_MODULE Container
#include <boost/test/unit_test.hpp>

#include "yandex/contest/system/execution/tests/BoostExecTest.hpp"

#include "yandex/contest/invoker/All.hpp"

namespace ya = yandex::contest::invoker;
typedef ya::ProcessGroup::Result PGR;
typedef ya::Process::Result PR;

struct ContainerFixture
{
    ContainerFixture():
        cfg_(ya::ContainerConfig::fromEnvironment()),
        cnt(ya::Container::create(cfg_)),
        pg(cnt->createProcessGroup())
    {
    }

    ya::ProcessPointer p(const std::size_t i, const boost::filesystem::path &executable)
    {
        p_.resize(std::max(i + 1, p_.size()));
        p_[i] = pg->createProcess(executable);
        return p(i);
    }

    template <typename ... Args>
    ya::ProcessPointer p(const std::size_t i, const boost::filesystem::path &executable, Args &&...args)
    {
        p(i, executable)->setArguments({executable.string(), std::forward<Args>(args)...});
        BOOST_REQUIRE_EQUAL(p(i)->arguments().size(), sizeof...(args) + 1);
        return p(i);
    }

    ya::ProcessPointer p(const std::size_t i)
    {
        BOOST_REQUIRE(i < p_.size());
        BOOST_REQUIRE(p_[i]);
        return p_[i];
    }

    void verifyPG(const PGR::CompletionStatus pgrcs)
    {
        PGR pgr = pg->wait();
        BOOST_CHECK_EQUAL(pgr.completionStatus, pgrcs);
    }

    void verify(const PGR::CompletionStatus pgrcs, const PR::CompletionStatus prcs)
    {
        verifyPG(pgrcs);
        for (const ya::ProcessPointer &pp: p_)
            if (pp)
                BOOST_CHECK_EQUAL(pp->result().completionStatus, prcs);
    }

#define VERIFY(STATUS) \
    void verify##STATUS() \
    { \
        verify(PGR::CompletionStatus::STATUS, PR::CompletionStatus::STATUS); \
    }

    VERIFY(OK)
    VERIFY(STOPPED)
    VERIFY(ABNORMAL_EXIT)

private:
    ya::ContainerConfig cfg_;

public:
    ya::ContainerPointer cnt;
    ya::ProcessGroupPointer pg;

    const char *const sleepTime = "0.1";
    const int sleepTimeMillis = 100;

private:
    std::vector<ya::ProcessPointer> p_;
};

#define CALL_CHECKPOINT(F) BOOST_TEST_CHECKPOINT(#F); F;

BOOST_FIXTURE_TEST_SUITE(Container, ContainerFixture)

BOOST_AUTO_TEST_SUITE(single)

BOOST_AUTO_TEST_CASE(true_)
{
    p(0, "true");
    CALL_CHECKPOINT(pg->start());
    verifyOK();
}

BOOST_AUTO_TEST_CASE(false_)
{
    p(0, "false");
    CALL_CHECKPOINT(pg->start());
    verifyABNORMAL_EXIT();
}

BOOST_AUTO_TEST_CASE(slow)
{
    ya::ProcessGroup::ResourceLimits pgrl;
    pgrl.realTimeLimitMillis = sleepTimeMillis / 2;
    p(0, "sleep", sleepTime);
    pg->setResourceLimits(pgrl);
    CALL_CHECKPOINT(pg->start());
    verifyPG(PGR::CompletionStatus::REAL_TIME_LIMIT_EXCEEDED);
}

BOOST_AUTO_TEST_CASE(freezing)
{
    p(0, "sleep", sleepTime);
    CALL_CHECKPOINT(pg->start());
    // note: this is not stable.
    // Increasing sleepTime can help
    // if process dies in timeout.
    CALL_CHECKPOINT(pg->freeze());
    CALL_CHECKPOINT(pg->unfreeze());
    verifyOK();
}

BOOST_AUTO_TEST_CASE(stop)
{
    p(0, "sleep", sleepTime);
    CALL_CHECKPOINT(pg->start());
    CALL_CHECKPOINT(pg->stop());
    verifySTOPPED();;
}

BOOST_AUTO_TEST_CASE(freeze_stop)
{
    p(0, "sleep", sleepTime);
    CALL_CHECKPOINT(pg->start());
    CALL_CHECKPOINT(pg->freeze());
    CALL_CHECKPOINT(pg->stop());
    verifySTOPPED();
}

BOOST_AUTO_TEST_CASE(freeze_wait)
{
    p(0, "sleep", sleepTime);
    pg->start();
    pg->freeze();
    BOOST_CHECK_THROW(pg->wait(), ya::ContainerIllegalStateError);
    pg->stop();
    verifySTOPPED();
}

BOOST_AUTO_TEST_CASE(dev_null_permissions)
{
    // we assume that /dev/null has 0666 permissions
    p(0, "sh", "-ce", "test `/usr/bin/env stat -c %a /dev/null` = 666");
    // and it is a character device
    p(0, "sh", "-ce", "test -c /dev/null");
    CALL_CHECKPOINT(pg->start());
    verifyOK();
}

BOOST_AUTO_TEST_SUITE_END() // single

BOOST_AUTO_TEST_SUITE_END() // Container
