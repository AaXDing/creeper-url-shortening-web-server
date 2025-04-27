#ifndef StaticRequestHandler_H
#define StaticRequestHandler_H

#include <string>

#include "http_header.h"
#include "request_handler.h"

class StaticRequestHandler : public RequestHandler {
 public:
  // Handle the Request and return the Response
  std::string handle_request(Request& req, Response& res) const override;

 private:
  std::string get_file_content_type(const std::string& file_path) const;
};

#endif  // StaticRequestHandler_H