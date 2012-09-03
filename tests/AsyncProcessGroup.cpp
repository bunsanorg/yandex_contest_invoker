#define BOOST_TEST_MODULE AsyncProcessGroup
#include <boost/test/unit_test.hpp>

#include "AsyncProcessGroupFixture.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

BOOST_FIXTURE_TEST_SUITE(AsyncProcessGroup, AsyncProcessGroupFixture)

BOOST_AUTO_TEST_CASE(serialization)
{
    PG::Task task0, task1;
    task0.processes.resize(1);
    // task.processes[0];
    task0.pipesNumber = 10;
    {
        std::ostringstream obuf;
        boost::archive::text_oarchive oa(obuf);
        oa << task0;
        std::istringstream ibuf(obuf.str());
        boost::archive::text_iarchive ia(ibuf);
        ia >> task1;
    }
    BOOST_CHECK_EQUAL(task1.pipesNumber, task0.pipesNumber);
    BOOST_CHECK_EQUAL(task1.processes.size(), task0.processes.size());
}

BOOST_AUTO_TEST_SUITE_END()
