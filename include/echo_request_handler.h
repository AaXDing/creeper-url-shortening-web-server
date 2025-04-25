#ifndef REQUEST_HANDLER_ECHO_H
#define REQUEST_HANDLER_ECHO_H

#include <string>

#include "http_header.h"
#include "request_handler.h"
#include "config_parser.h"

class EchoRequestHandlerTest;  // forward declaration for test fixture

class EchoRequestHandler: public RequestHandler {
  /*
      This class handles HTTP requests by echoing back the request message.
      It inherits from RequestHandler and implements the handle_request method.
  */
 public:
  virtual void handle_request(Request& req, Response& res) const;
  virtual std::string response_to_string(const Response& res) const;
  friend class EchoRequestHandlerTest;

 private:
  virtual std::string request_to_string(const Request& req) const;
};

#endif  // REQUEST_HANDLER_ECHO_H