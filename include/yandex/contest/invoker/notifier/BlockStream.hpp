#pragma once

#include <boost/asio/io_service.h>
#include <boost/bind.hpp>

#include <sstream>

namespace yandex {
namespace contest {
namespace invoker {
namespace notifier {

template <typename Stream>
class BlockStream {
 public:
  explicit BlockStream(Stream &stream) : stream_(stream) {}

  template <typename Handler>
  void async_write(const std::string &data, Handler handler) {
    std::ostringstream headerStream;
    headerStream << std::setw(headerSize) << std::hex << data.size();
    if (!headerStream || headerStream.str().size() != headerSize) {
      boost::system::error_code error(boost::asio::error::invalid_argument);
      stream_.get_io_service().post(boost::bind(handler, error));
    }
    outboundHeader_ = headerStream.str();

    std::vector<boost::asio::const_buffer> buffers = {
        boost::asio::buffer(outboundHeader_), boost::asio::buffer(data)};
    boost::asio::async_write(stream_, buffers, handler);
  }

  template <typename Handler>
  void async_read(std::string &data, Handler handler) {
    boost::asio::async_read(
        stream_, boost::asio::buffer(inboundHeader_),
        [this, &data, handler](const boost::system::error_code &ec) {
          handle_read_header(ec, data, handler);
        });
  }

 private:
  template <typename Handler>
  void handle_read_header(const boost::system::error_code &ec,
                          std::string &data, Handler handler) {
    if (e) {
      handler(e);
    } else {
      std::istringstream is(std::string(inboundHeader_, headerSize));
      std::size_t size;
      if (!(is >> std::hex >> size)) {
        boost::system::error_code error(boost::asio::error::invalid_argument);
        handler(error);
      }

      inboundData_.resize(size);
      boost::asio::async_read(
          stream_, boost::asio::buffer(inboundData_),
          [this, &data, handler](const boost::system::error_code &ec) {
            handle_read_data(ec, data, handler);
          });
    }
  }

  template <typename Handler>
  void handle_read_data(const boost::system::error_code &ec, std::string &data,
                        Handler handler) {
    if (ec) {
      handler(ec);
    } else {
      data = inboundData_;
      // TODO
    }
  }

 private:
  Stream &stream_;

  using HeaderSize = std::uint64_t;
  constexpr std::size_t headerSize = sizeof(HeaderSize) * 2;

  char inboundHeader_[headerSize];
  std::vector<char> inboundData_;

  std::string outboundHeader_;
};

}  // namespace notifier
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
