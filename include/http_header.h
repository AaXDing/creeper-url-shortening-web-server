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
    {"css", "text/css"},        {"js", "application/javascript"},
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
  bool valid = false;  // default valid to false

  std::string to_string() const;
};

struct Response {
  // HTTP Response line
  std::string version;
  int status_code;
  std::string status_message;
  std::vector<Header> headers;
  std::string body;

  Response();
  Response(std::string version, int status_code, std::string status_message,
           std::vector<Header> headers, std::string body);
  std::string to_string() const;
};

const std::unordered_map<unsigned int, Response> STOCK_RESPONSE = {
    {400, Response(HTTP_VERSION, 400, "Bad Request",
                   {{"Content-Type", "text/plain"}}, "400 Bad Request")},
    {404, Response(HTTP_VERSION, 404, "Not Found",
                   {{"Content-Type", "text/plain"}}, "404 Not Found")},
    {405, Response(HTTP_VERSION, 405, "Method Not Allowed",
                   {{"Content-Type", "text/plain"}}, "405 Method Not Allowed")},
    {415,
     Response(HTTP_VERSION, 415, "Unsupported Media Type",
              {{"Content-Type", "text/plain"}}, "415 Unsupported Media Type")},
    {500,
     Response(HTTP_VERSION, 500, "Internal Server Error",
              {{"Content-Type", "text/plain"}}, "500 Internal Server Error")},
};

#endif  // HTTP_HEADER_H