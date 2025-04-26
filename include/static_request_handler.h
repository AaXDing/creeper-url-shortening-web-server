#ifndef StaticRequestHandler_H
#define StaticRequestHandler_H

#include <string>

#include "http_header.h"
#include "request_handler.h"

class StaticRequestHandler : public RequestHandler {
 public:
  // Handle the Request and return the Response
  std::string handle_request(Request& req, Response& res) const override;
};

#endif  // StaticRequestHandler_H