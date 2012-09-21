#pragma once

#include "yandex/contest/tests/BoostExecTest.hpp"

#include "yandex/contest/invoker/All.hpp"

namespace ya = yandex::contest::invoker;

struct ContainerFixture
{
    typedef ya::ProcessGroup::Result PGR;
    typedef ya::Process::Result PR;

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
            {
                BOOST_TEST_MESSAGE("Process [" << pp->id() << "]:");
                BOOST_CHECK_EQUAL(pp->result().completionStatus, prcs);
                if (pp->result().exitStatus)
                    BOOST_TEST_MESSAGE("ExitStatus: " << pp->result().exitStatus.get());
                if (pp->result().termSig)
                    BOOST_TEST_MESSAGE("TermSig: " << pp->result().termSig.get());
            }
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

    const char *const sleepTimeStr = "0.2";
    const std::chrono::milliseconds sleepTime = std::chrono::milliseconds(200);

private:
    std::vector<ya::ProcessPointer> p_;
};
