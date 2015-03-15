#pragma once

#include <yandex/contest/invoker/All.hpp>

#include <bunsan/testing/environment.hpp>
#include <bunsan/testing/exec_test.hpp>

namespace ya = yandex::contest::invoker;

struct ContainerFixture
{
    typedef ya::ProcessGroup::Result PGR;
    typedef ya::Process::Result PR;

    ContainerFixture():
        cfg_(getContainerConfig()),
        cnt(ya::Container::create(cfg_)),
        pg(cnt->createProcessGroup())
    {
    }

    ya::ProcessPointer p(
        const std::size_t i,
        const boost::filesystem::path &executable)
    {
        p_.resize(std::max(i + 1, p_.size()));
        p_[i].process = pg->createProcess(executable);
        return p(i);
    }

    template <typename ... Args>
    ya::ProcessPointer p(
        const std::size_t i,
        const boost::filesystem::path &executable, Args &&...args)
    {
        p(i, executable)->setArguments({
            executable.string(),
            std::forward<Args>(args)...
        });
        BOOST_REQUIRE_EQUAL(p(i)->arguments().size(), sizeof...(args) + 1);
        return p(i);
    }

    ya::ProcessPointer p(const std::size_t i)
    {
        BOOST_REQUIRE(i < p_.size());
        BOOST_REQUIRE(p_[i].process);
        return p_[i].process;
    }

    template <typename ... Args>
    ya::ProcessPointer pn(
        const std::size_t i,
        const std::string &name, Args &&...args)
    {
        p(i, std::forward<Args>(args)...);
        p_[i].name = name;
        return p(i);
    }

    void verifyPG(const PGR::CompletionStatus pgrcs)
    {
        PGR pgr = pg->wait();
        BOOST_CHECK_EQUAL(pgr.completionStatus, pgrcs);
    }

    void infoP(const std::size_t i)
    {
        const ya::ProcessPointer pp = p(i);
        BOOST_TEST_MESSAGE("CompletionStatus: " << pp->result().completionStatus);
        if (pp->result().exitStatus)
            BOOST_TEST_MESSAGE("ExitStatus: " << pp->result().exitStatus.get());
        if (pp->result().termSig)
            BOOST_TEST_MESSAGE("TermSig: " << pp->result().termSig.get());
    }

    void verifyP(
        const std::size_t i,
        const PR::CompletionStatus prcs=PR::CompletionStatus::OK)
    {
        BOOST_ASSERT(i < p_.size());
        const ya::ProcessPointer pp = p_[i].process;
        if (pp)
        {
            if (p_[i].name.empty())
                BOOST_TEST_MESSAGE("Process [" << pp->id() << "]:");
            else
                BOOST_TEST_MESSAGE("Process [" << p_[i].name << "]:");
            BOOST_CHECK_EQUAL(pp->result().completionStatus, prcs);
            infoP(i);
        }
    }

    void verify(
        const PGR::CompletionStatus pgrcs,
        const PR::CompletionStatus prcs)
    {
        verifyPG(pgrcs);
        for (std::size_t i = 0; i < p_.size(); ++i)
            verifyP(i, prcs);
    }

#define VERIFY(STATUS) \
    void verify##STATUS() \
    { \
        verify( \
            PGR::CompletionStatus::STATUS, \
            PR::CompletionStatus::STATUS); \
    }

    VERIFY(OK)
    VERIFY(STOPPED)
    VERIFY(ABNORMAL_EXIT)

    static ya::ContainerConfig getContainerConfig()
    {
        ya::ContainerConfig config = ya::ContainerConfig::fromEnvironment();
        config.controlProcessConfig.executable = bunsan::testing::dir::binary() / "yandex_contest_invoker_ctl";
        { // fix mounts
            config.lxcConfig.mount->entries->push_back(yandex::contest::system::unistd::MountEntry::bindRO(
                bunsan::testing::dir::binary(),
                bunsan::testing::dir::binary()
            ));
        }
        return config;
    }

private:
    ya::ContainerConfig cfg_;

public:
    ya::ContainerPointer cnt;
    ya::ProcessGroupPointer pg;

    const char *const sleepTimeStr = "0.2";
    const std::chrono::milliseconds sleepTime =
        std::chrono::milliseconds(200);

private:
    struct Process
    {
        ya::ProcessPointer process;
        std::string name;
    };

    std::vector<Process> p_;
};
