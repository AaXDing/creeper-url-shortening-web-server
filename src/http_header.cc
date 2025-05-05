#include "http_header.h"
#include "logging.h"

#include <string>

std::string Request::to_string() const {
  
  LOG(debug) << "Serializing request to string";

  std::string request_str = method + " " + uri + " " + version + CRLF;
  for (const auto& header : headers) {
    request_str += header.name + ": " + header.value + CRLF;
  }
  request_str += CRLF;
  return request_str;
}

Response::Response() {}

Response::Response(std::string version, int status_code,
                   std::string status_message, std::string content_type,
                   std::string body)
    : version(std::move(version)),
      status_code(status_code),
      status_message(std::move(status_message)),
      content_type(std::move(content_type)),
      body(std::move(body)) {}

std::string Response::to_string() const {
  LOG(debug) << "Serializing response to string; length=" << body.size();
  std::string response_str =
      "HTTP/1.1 " + std::to_string(status_code) + " " + status_message + CRLF;
  response_str += "Content-Type: " + content_type + CRLF;
  response_str += "Content-Length: " + std::to_string(body.size()) + CRLF;
  response_str += CRLF;
  response_str += body;
  return response_str;
}