#pragma once

#include <yandex/contest/invoker/All.hpp>
#include <yandex/contest/invoker/test/ContainerConfig.hpp>

#include <bunsan/test/exec_test.hpp>

namespace ya = yandex::contest::invoker;

struct ContainerFixture {
  using PGR = ya::ProcessGroup::Result;
  using PR = ya::Process::Result;

  ContainerFixture() : cfg(ya::test::getContainerConfig()) { resetContainer(); }

  void resetContainer() {
    cnt = ya::Container::create(cfg);
    pg = cnt->createProcessGroup();
    p_.clear();
  }

  ya::ProcessPointer p(const std::size_t i,
                       const boost::filesystem::path &executable) {
    p_.resize(std::max(i + 1, p_.size()));
    p_[i].process = pg->createProcess(executable);
    return p(i);
  }

  template <typename... Args>
  ya::ProcessPointer p(const std::size_t i,
                       const boost::filesystem::path &executable,
                       Args &&... args) {
    p(i, executable)
        ->setArguments({executable.string(), std::forward<Args>(args)...});
    BOOST_REQUIRE_EQUAL(p(i)->arguments().size(), sizeof...(args) + 1);
    return p(i);
  }

  ya::ProcessPointer p(const std::size_t i) {
    BOOST_REQUIRE(i < p_.size());
    BOOST_REQUIRE(p_[i].process);
    return p_[i].process;
  }

  template <typename... Args>
  ya::ProcessPointer pn(const std::size_t i, const std::string &name,
                        Args &&... args) {
    p(i, std::forward<Args>(args)...);
    p_[i].name = name;
    return p(i);
  }

  void verifyPG(const PGR::CompletionStatus pgrcs) {
    PGR pgr = pg->wait();
    BOOST_CHECK_EQUAL(pgr.completionStatus, pgrcs);
  }

  void infoP(const std::size_t i) {
    const ya::ProcessPointer pp = p(i);
    BOOST_TEST_MESSAGE("CompletionStatus: " << pp->result().completionStatus);
    if (pp->result().exitStatus)
      BOOST_TEST_MESSAGE("ExitStatus: " << pp->result().exitStatus.get());
    if (pp->result().termSig)
      BOOST_TEST_MESSAGE("TermSig: " << pp->result().termSig.get());
  }

  void verifyP(const std::size_t i,
               const PR::CompletionStatus prcs = PR::CompletionStatus::OK) {
    BOOST_ASSERT(i < p_.size());
    const ya::ProcessPointer pp = p_[i].process;
    if (pp) {
      if (p_[i].name.empty())
        BOOST_TEST_MESSAGE("Process [" << pp->id() << "]:");
      else
        BOOST_TEST_MESSAGE("Process [" << p_[i].name << "]:");
      BOOST_CHECK_EQUAL(pp->result().completionStatus, prcs);
      infoP(i);
    }
  }

  void verify(const PGR::CompletionStatus pgrcs,
              const PR::CompletionStatus prcs) {
    verifyPG(pgrcs);
    for (std::size_t i = 0; i < p_.size(); ++i) verifyP(i, prcs);
  }

#define VERIFY(STATUS)                                                   \
  void verify##STATUS() {                                                \
    verify(PGR::CompletionStatus::STATUS, PR::CompletionStatus::STATUS); \
  }

  VERIFY(OK)
  VERIFY(STOPPED)
  VERIFY(ABNORMAL_EXIT)

  ya::ContainerConfig cfg;
  ya::ContainerPointer cnt;
  ya::ProcessGroupPointer pg;

  const char *const sleepTimeStr = "0.2";
  const std::chrono::milliseconds sleepTime = std::chrono::milliseconds(200);

 private:
  struct Process {
    ya::ProcessPointer process;
    std::string name;
  };

  std::vector<Process> p_;
};
