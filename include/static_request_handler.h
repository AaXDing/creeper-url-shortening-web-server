#ifndef STATIC_REQUEST_HANDLER_H
#define STATIC_REQUEST_HANDLER_H

#include <string>

#include "http_header.h"
#include "request_handler.h"

class StaticRequestHandlerTest;  // forward declaration for test fixture

class StaticRequestHandler : public RequestHandler {
 public:
  // Factory method to create a StaticRequestHandler
  static StaticRequestHandler* create(std::string base_uri,
                                      std::string root_path);
  // Handle the Request and return the Response
  Response handle_request(Request& req) const override;
  RequestHandler::HandlerType get_type() const override;

  friend class StaticRequestHandlerTest;

 private:
  StaticRequestHandler(std::string base_uri, std::string root_path);
  std::string generate_file_path(const std::string& file_path) const;
  std::string get_file_content_type(const std::string& file_path) const;
  std::string base_uri_;
  std::string root_path_;
};

#endif  // STATIC_REQUEST_HANDLER_H