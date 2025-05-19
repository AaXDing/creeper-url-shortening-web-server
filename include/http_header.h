#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include <string>
#include <unordered_map>
#include <vector>

const std::string CRLF = "\r\n";
const std::string HTTP_VERSION = "HTTP/1.1";
const std::string METHOD_GET = "GET";
const std::unordered_map<std::string, std::string> CONTENT_TYPE = {
    {"html", "text/html"},      {"txt", "text/plain"},
    {"pdf", "application/pdf"}, {"zip", "application/zip"},
    {"jpeg", "image/jpeg"},     {"jpg", "image/jpeg"},
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
  std::string body;
  bool valid = false; // default valid to false

  std::string to_string() const;
};

struct Response {
  // HTTP Response line
  std::string version;
  int status_code;
  std::string status_message;
  std::string content_type;
  std::string body;

  Response();
  Response(std::string version, int status_code, std::string status_message,
           std::string content_type, std::string);
  std::string to_string() const;
};

const std::unordered_map<unsigned int, Response> STOCK_RESPONSE = {
    {400, Response(HTTP_VERSION, 400, "Bad Request", "text/plain",
                   "400 Bad Request")},
    {404,
     Response(HTTP_VERSION, 404, "Not Found", "text/plain", "404 Not Found")},
    {415, Response(HTTP_VERSION, 415, "Unsupported Media Type", "text/plain",
                   "415 Unsupported Media Type")},
    {500, Response(HTTP_VERSION, 500, "Internal Server Error", "text/plain",
                   "500 Internal Server Error")},
};

#endif // HTTP_HEADER_H