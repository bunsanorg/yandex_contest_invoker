#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/io_service.h>

#include <boost/archive/text_oarchive.hpp>

#include <sstream>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

template <typename Stream>
class ObjectStream {
 public:
  explicit ObjectStream(Stream &stream) : blockStream_(stream) {}

  template <typename T, typename Handler>
  void async_write(const T &obj, Handler handler) {
    std::ostringstream sout;
    {
      boost::archive::text_oarchive oa(sout);
      oa << obj;
    }
    blockStream_.async_write(sout.str(), handler);
  }

  template <typename T, typename Handler>
  void async_read(T &obj, Handler handler) {
    blockStream_.han
  }

 private:
  BlockStream<Stream> blockStream_;
  std::string data_;
};

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
