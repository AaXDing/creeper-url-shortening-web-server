#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include <string>
#include <unordered_map>
#include <vector>

const std::string CRLF = "\r\n";
const std::string HTTP_VERSION = "HTTP/1.1";
const std::string METHOD_GET = "GET";
const std::unordered_map<unsigned int, std::string> STOCK_RESPONSE = {
    {400,
     "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: "
     "15\r\n\r\n400 Bad Request"},
    {404,
     "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: "
     "13\r\n\r\n404 Not Found"},
};

struct Header {
  std::string name;
  std::string value;
};

struct Request {
  // HTTP Request line
  std::string method;
  std::string uri;
  std::string version;
  std::vector<Header> headers;
  bool valid = false;  // default valid to false
};

struct Response {
  // HTTP Response line
  std::string version;
  int status_code;
  std::string status_message;
  std::string content_type;
  // std::string content_length;
  std::string body;
};

#endif  // HTTP_HEADER_H