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
  virtual std::string handle_request(Request& req, Response& res) const = 0;

 protected:
  std::string response_to_string(const Response& res) const;
};

#endif