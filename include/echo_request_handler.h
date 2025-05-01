#ifndef ECHO_REQUEST_HANDLER_H
#define ECHO_REQUEST_HANDLER_H

#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"

class EchoRequestHandlerTest;  // forward declaration for test fixture

class EchoRequestHandler : public RequestHandler {
  /*
      This class handles HTTP requests by echoing back the request message.
      It inherits from RequestHandler and implements the handle_request method.
  */
 public:
  virtual Response handle_request(Request& req) const;

  friend class EchoRequestHandlerTest;

 private:
  std::string request_to_string(const Request& req) const;
};

#endif  // ECHO_REQUEST_HANDLER_H