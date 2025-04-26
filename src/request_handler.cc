#include "request_handler.h"

std::string RequestHandler::response_to_string(const Response& res) const {
  // Construct the HTTP response string from the Response object
  std::string response = "HTTP/1.1 " + std::to_string(res.status_code) + " " +
                         res.status_message + "\r\n";
  response += "Content-Type: text/plain\r\n";
  response += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
  response += "\r\n";
  response += res.body;
  return response;
}