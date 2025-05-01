#include "http_header.h"

#include <string>

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
  std::string response_str =
      "HTTP/1.1 " + std::to_string(status_code) + " " + status_message + CRLF;
  response_str += "Content-Type: " + content_type + CRLF;
  response_str += "Content-Length: " + std::to_string(body.size()) + CRLF;
  response_str += CRLF;
  response_str += body;
  return response_str;
}