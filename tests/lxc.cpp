#define BOOST_TEST_MODULE lxc
#include <boost/test/unit_test.hpp>

#include <yandex/contest/invoker/lxc/ConfigHelper.hpp>
#include <yandex/contest/invoker/lxc/LxcApi.hpp>

#include <lxc/lxccontainer.h>

#include <sstream>

BOOST_AUTO_TEST_SUITE(ConfigHelper)

namespace ya = yandex::contest::invoker::lxc;
namespace cfg = ya::config_helper;

BOOST_AUTO_TEST_SUITE(api)

BOOST_AUTO_TEST_CASE(ptr) {
  const ya::api::container_ptr ptr =
      ya::api::container_new("hello", "/some/path");
  BOOST_CHECK_EQUAL(ptr->numthreads, 1);
  const auto ptr2 = ptr;
  BOOST_CHECK_EQUAL(ptr2->numthreads, 2);
}

BOOST_AUTO_TEST_SUITE_END()  // api

BOOST_AUTO_TEST_CASE(general) {
  const std::string key = "key";
  const std::string value = "value";
  std::ostringstream buf;
  cfg::output(buf, key, value);
  BOOST_CHECK_EQUAL(buf.str(), key + " = " + value + "\n");
}

BOOST_AUTO_TEST_CASE(exception) {
  const std::string key = "ke=y";
  const std::string value = "value";
  std::ostringstream buf;
  BOOST_CHECK_THROW(cfg::output(buf, key, value), ya::ConfigError);
}

BOOST_AUTO_TEST_CASE(optional) {
  const std::string key = "key";
  const std::string value = "value";
  {
    std::ostringstream buf;
    cfg::optionalOutput(buf, key, boost::optional<std::string>(value));
    BOOST_CHECK_EQUAL(buf.str(), key + " = " + value + "\n");
  }
  {
    std::ostringstream buf;
    cfg::optionalOutput(buf, key, boost::optional<std::string>());
    BOOST_CHECK_EQUAL(buf.str(), "");
  }
}

BOOST_AUTO_TEST_SUITE_END()  // ConfigHelper
