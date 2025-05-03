#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>

#include "http_header.h"

class RequestHandler {
  /*
      This is an abstract class that defines the interface for handling
      requests. It contains a pure virtual function handle_request() that must
      be implemented by any derived class. The function is responsible for
      processing the request and generating a response.
  */
 public:
  enum class HandlerType {
    ECHO_REQUEST_HANDLER,
    STATIC_REQUEST_HANDLER,
  };  // Enum to represent the type of handler

  virtual Response handle_request(Request& req) const = 0;
  virtual HandlerType get_type() const = 0;
};

#endif