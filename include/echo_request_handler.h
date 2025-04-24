#ifndef REQUEST_HANDLER_ECHO_H
#define REQUEST_HANDLER_ECHO_H

#include <string>

#include "http_header.h"

class EchoRequestHandlerTest;  // forward declaration for test fixture

class EchoRequestHandler {
 public:
  // Handle the Request and return the Response
  void handle_request(Request& req, Response& res) const;
  std::string response_to_string(const Response& res) const;
  friend class EchoRequestHandlerTest;

 private:
  std::string request_to_string(const Request& req) const;
};

#endif  // REQUEST_HANDLER_ECHO_H