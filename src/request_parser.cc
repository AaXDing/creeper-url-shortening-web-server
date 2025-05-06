#include "request_parser.h"
#include "logging.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <unordered_set>

std::string get_version(unsigned int version) {
  return "HTTP/" + std::to_string(version / 10) + "." +
         std::to_string(version % 10);
}

const std::unordered_set<std::string> allowed_methods = {
    "GET"  // only GET method is allowed for now
           /* future methods to be added:
           , "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
           */
};

void RequestParser::parse(Request &req, const std::string &raw_request) {
  // Store raw http Request to parser
  boost::beast::flat_buffer buffer;
  buffer.commit(boost::asio::buffer_copy(buffer.prepare(raw_request.size()),
                                         boost::asio::buffer(raw_request)));

  boost::beast::http::request_parser<boost::beast::http::string_body> parser;
  boost::beast::error_code ec;

  parser.put(buffer.data(), ec);

  // If error in Request, Request not valid
  if (ec) {
    LOG(error) << "HTTP parse error: " << ec.message();
    req.valid = false;
    return;
  }

  // Get parsed Request
  auto res = parser.get();
  LOG(trace) << "parsed version=" << res.version() << " method=" << res.method_string();

  // Check Request version HTTP/1.1 and Request method allowed
  if (res.version() != 11 ||
      allowed_methods.find(res.method_string()) == allowed_methods.end()) {
    LOG(error) << "Invalid HTTP version or method: " << res.version() << " "
               << res.method_string();
    req.valid = false;
    return;
  }
  req.valid = true;
  req.version = get_version(res.version());
  req.uri = res.target();
  req.method = res.method_string();

  LOG(info) << "Valid request: " << req.method << " " << req.uri << " ("
            << req.version << ")";

  for (const auto &field : res) {
    req.headers.push_back(
        Header{std::string(field.name_string()), std::string(field.value())});
  }
}
