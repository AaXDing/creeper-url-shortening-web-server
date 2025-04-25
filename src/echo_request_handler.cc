// A class that handles HTTP requests

#include "echo_request_handler.h"
#include "config_parser.h"

void EchoRequestHandler::handle_request(Request& req, Response& res) const {
  if (req.valid) {  // If the request is valid, echo the request
    res.status_code = 200;
    res.status_message = "OK";
    res.version = req.valid ? req.version : HTTP_VERSION;
    ;
    res.content_type = "text/plain";
    res.body = request_to_string(req);
  } else {  // If the request is invalid, return a 400 Bad Request response
    res.status_code = 400;
    res.status_message = "Bad Request";
    res.version = HTTP_VERSION;
    res.content_type = "text/plain";
    res.body = "400 Bad Request";
  }
}

std::string EchoRequestHandler::response_to_string(const Response& res) const {
  // Construct the HTTP response string from the Response object
  std::string response = "HTTP/1.1 " + std::to_string(res.status_code) + " " +
                         res.status_message + "\r\n";
  response += "Content-Type: text/plain\r\n";
  response += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
  response += "\r\n";
  response += res.body;
  return response;
}

std::string EchoRequestHandler::request_to_string(const Request& req) const {
  // Construct the HTTP request string from the Request object
  std::string request_str =
      req.method + " " + req.uri + " " + req.version + "\r\n";
  for (const auto& header : req.headers) {
    request_str += header.name + ": " + header.value + "\r\n";
  }
  request_str += "\r\n";
  return request_str;
}