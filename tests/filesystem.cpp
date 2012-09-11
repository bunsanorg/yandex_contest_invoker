#define BOOST_TEST_MODULE filesystem
#include <boost/test/unit_test.hpp>

#include <iterator>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

#include "yandex/contest/system/unistd/Operations.hpp"

#include "yandex/contest/invoker/filesystem/Operations.hpp"
#include "yandex/contest/invoker/filesystem/Config.hpp"
#include "yandex/contest/invoker/filesystem/CreateFile.hpp"
#include "yandex/contest/invoker/filesystem/File.hpp"

namespace ya = yandex::contest::invoker::filesystem;
namespace unistd = yandex::contest::system::unistd;

BOOST_AUTO_TEST_SUITE(Operations)

BOOST_AUTO_TEST_CASE(keepInRoot)
{
    const boost::filesystem::path roots[] = {"/srv/lxc", "srv/lxc"};
    for (const boost::filesystem::path &root: roots)
    {
        BOOST_CHECK_EQUAL(ya::keepInRoot("some/path", root), root / "some/path");
        BOOST_CHECK_EQUAL(ya::keepInRoot("/some/path", root), root / "/some/path");
        BOOST_CHECK_EQUAL(ya::keepInRoot("../../../../etc/passwd", root), root / "etc/passwd");
        BOOST_CHECK_EQUAL(ya::keepInRoot("/../../../../etc/passwd", root), root / "/etc/passwd");
    }
}

BOOST_AUTO_TEST_SUITE_END()

struct CreateFileFixture
{
    CreateFileFixture()
    {
        BOOST_REQUIRE_EQUAL(unistd::getuid(), 0);
        root = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
        BOOST_REQUIRE(boost::filesystem::create_directory(root));
        source = root / "source";
        path = root / "path";
        target = root / "target";
    }

    void verifyStatus(const ya::File &file, const boost::filesystem::path &path)
    {
        BOOST_REQUIRE(boost::filesystem::exists(path));
        const unistd::FileStatus status = unistd::stat(path);
        BOOST_CHECK_EQUAL(status.permissions(), file.mode);
        BOOST_CHECK_EQUAL(status.ownerId, file.ownerId);
    }

    ~CreateFileFixture()
    {
        boost::filesystem::remove_all(root);
    }

    boost::filesystem::path root, source, path, target;
};

BOOST_FIXTURE_TEST_SUITE(CreateFile, CreateFileFixture)

BOOST_AUTO_TEST_CASE(RegularFileEmpty)
{
    ya::RegularFile file;
    file.path = path;
    file.mode = 0735;
    file.create();
    verifyStatus(file, path);
}

BOOST_AUTO_TEST_CASE(RegularFileSource)
{
    boost::filesystem::ofstream fout(source);
    BOOST_REQUIRE(fout);
    const std::string s = "Hello, world!";
    fout << s;
    fout.close();
    BOOST_REQUIRE(fout);
    BOOST_REQUIRE(boost::filesystem::exists(source));
    ya::RegularFile file;
    file.path = target;
    file.mode = 0375;
    file.source = source;
    file.create();
    verifyStatus(file, target);
    boost::filesystem::ifstream fin(target);
    BOOST_REQUIRE(fin);
    const std::string r{std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>()};
    fin.close();
    BOOST_REQUIRE(fin);
    BOOST_CHECK_EQUAL(r, s);
}

BOOST_AUTO_TEST_CASE(RegularFileOverwrite)
{
    boost::filesystem::ofstream fout(target);
    BOOST_REQUIRE(fout);
    fout << "will not be empty";
    fout.close();
    BOOST_REQUIRE(fout);
    BOOST_REQUIRE(boost::filesystem::exists(target));
    fout.open(source);
    BOOST_REQUIRE(fout);
    const std::string s = "Hello, world!";
    fout << s;
    fout.close();
    BOOST_REQUIRE(fout);
    BOOST_REQUIRE(boost::filesystem::exists(source));
    ya::RegularFile file;
    file.path = target;
    file.mode = 0735;
    file.source = source;
    file.create();
    verifyStatus(file, target);
    boost::filesystem::ifstream fin(target);
    BOOST_REQUIRE(fin);
    const std::string r{std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>()};
    fin.close();
    BOOST_REQUIRE(fin);
    BOOST_CHECK_EQUAL(r, s);
}

BOOST_AUTO_TEST_CASE(Directory)
{
    ya::Directory dir;
    dir.path = path;
    dir.mode = 0247;
    BOOST_REQUIRE(!boost::filesystem::exists(path));
    dir.create();
    BOOST_CHECK(boost::filesystem::is_directory(path));
    verifyStatus(dir, path);
}

BOOST_AUTO_TEST_CASE(DirectoryOverwrite)
{
    ya::Directory dir;
    dir.path = path;
    dir.mode = 0174;
    dir.ownerId = {654, 321};
    boost::filesystem::create_directory(path);
    BOOST_REQUIRE(boost::filesystem::is_directory(path));
    dir.create();
    BOOST_REQUIRE(boost::filesystem::is_directory(path));
    verifyStatus(dir, path);
}

BOOST_AUTO_TEST_CASE(SymLink)
{
    ya::SymLink link;
    link.path = target;
    link.value = source;
    link.ownerId = {123, 456};
    link.create();
    BOOST_CHECK(boost::filesystem::is_symlink(target));
    BOOST_CHECK_EQUAL(boost::filesystem::read_symlink(target), link.value);
    BOOST_CHECK_EQUAL(unistd::lstat(target).ownerId, link.ownerId);
}

BOOST_AUTO_TEST_CASE(Device)
{
    ya::Device dev;
    dev.path = path;
    dev.mode = 0734;
    // /dev/null
    dev.type = ya::Device::CHAR;
    dev.minor = 1;
    dev.major = 3;
    dev.create();
    verifyStatus(dev, path);
}

BOOST_AUTO_TEST_CASE(File__createDirectories)
{
    const boost::filesystem::path dir = path;
    BOOST_REQUIRE(!boost::filesystem::exists(dir));
    const boost::filesystem::path fpath = dir / "fpath";
    ya::RegularFile file;
    file.path = fpath;
    file.mode = 0123;
    file.ownerId = {123, 456};
    file.create();
    verifyStatus(file, fpath);
}

BOOST_AUTO_TEST_CASE(CreateFile__create0)
{
    ya::CreateFile create;
    ya::RegularFile file;
    file.path = path;
    create = file;
    create.create();
    BOOST_CHECK(boost::filesystem::exists(path));
}

BOOST_AUTO_TEST_CASE(CreateFile__create1)
{
    ya::CreateFile create;
    ya::RegularFile file;
    file.path = path.filename();
    create = file;
    create.create(root);
    BOOST_CHECK(boost::filesystem::exists(path));
}

BOOST_AUTO_TEST_SUITE_END()
