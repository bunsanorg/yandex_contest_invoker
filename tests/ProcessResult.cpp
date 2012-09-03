#define BOOST_TEST_MODULE ProcessResult
#include <boost/test/unit_test.hpp>

#include "yandex/contest/invoker/process/Result.hpp"

namespace ya = yandex::contest::invoker::process;

BOOST_AUTO_TEST_SUITE(Result)

BOOST_AUTO_TEST_CASE(default_ctor)
{
    ya::Result pr;
    BOOST_CHECK(!pr);
    pr.exitStatus = 0;
    BOOST_CHECK(pr);
}

BOOST_AUTO_TEST_CASE(operator_bool)
{
    ya::Result pr;
    BOOST_CHECK(!pr);
    pr.exitStatus = 0;
    BOOST_CHECK(pr);
    pr.completionStatus = ya::Result::CompletionStatus::START_FAILED;
    BOOST_CHECK(!pr);
    pr.completionStatus = ya::Result::CompletionStatus::OK;
    BOOST_CHECK(pr);
    pr.termSig = 9;
    BOOST_CHECK(!pr);
    pr.exitStatus = 1;
    BOOST_CHECK(!pr);
    pr.termSig.reset();
    BOOST_CHECK(!pr);
}

BOOST_AUTO_TEST_SUITE_END()
