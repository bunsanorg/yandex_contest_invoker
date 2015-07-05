#define BOOST_TEST_MODULE Container
#include <boost/test/unit_test.hpp>

#include <yandex/contest/invoker/tests/ContainerFixture.hpp>

#include <bunsan/test/filesystem/read_data.hpp>
#include <bunsan/test/filesystem/tempdir.hpp>
#include <bunsan/test/filesystem/tempfile.hpp>
#include <bunsan/test/filesystem/write_data.hpp>

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
    pgrl.realTimeLimit = sleepTime / 2;
    p(0, "sleep", sleepTimeStr);
    pg->setResourceLimits(pgrl);
    CALL_CHECKPOINT(pg->start());
    verifyPG(PGR::CompletionStatus::REAL_TIME_LIMIT_EXCEEDED);
}

BOOST_AUTO_TEST_CASE(stop)
{
    p(0, "sleep", sleepTimeStr);
    CALL_CHECKPOINT(pg->start());
    CALL_CHECKPOINT(pg->stop());
    verifySTOPPED();;
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

BOOST_AUTO_TEST_CASE(mount_regular_file)
{
    bunsan::test::filesystem::tempfile tmp;
    cfg.lxcConfig.mount->entries->push_back(
        yandex::contest::system::unistd::MountEntry::bind(tmp.path, "/some/strange/path")
    );
    resetContainer();
    bunsan::test::filesystem::write_data(tmp.path, "hello world");
    p(0, "sh", "-ce", "test \"$(cat /some/strange/path)\" = 'hello world'");
    CALL_CHECKPOINT(pg->start());
    verifyOK();
}

BOOST_AUTO_TEST_CASE(mount_directory)
{
    bunsan::test::filesystem::tempdir tmp;
    cfg.lxcConfig.mount->entries->push_back(
        yandex::contest::system::unistd::MountEntry::bind(tmp.path, "/some/strange/path")
    );
    resetContainer();
    bunsan::test::filesystem::write_data(tmp.path / "file", "hello world");
    p(0, "sh", "-ce", "test \"$(cat /some/strange/path/file)\" = 'hello world'");
    CALL_CHECKPOINT(pg->start());
    verifyOK();
}

BOOST_AUTO_TEST_SUITE_END() // single

BOOST_AUTO_TEST_SUITE_END() // Container
