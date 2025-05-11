#ifndef STATIC_REQUEST_HANDLER_H
#define STATIC_REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"
class StaticRequestHandlerTest;  // forward declaration for test fixture

class StaticRequestHandler : public RequestHandler {
 public:
  // Factory method to create a unique_ptr to a StaticRequestHandler
  static std::unique_ptr<StaticRequestHandler> create(std::string base_uri,
                                                      std::string root_path);

  StaticRequestHandler(std::string base_uri, std::string root_path);
  // Handle the Request and return Response
  std::unique_ptr<Response> handle_request(const Request& req) override;
  static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                             NginxLocation& location);
  RequestHandler::HandlerType get_type() const override;

  friend class StaticRequestHandlerTest;

 private:
  std::string generate_file_path(const std::string& file_path) const;
  std::string get_file_content_type(const std::string& file_path) const;
  std::string base_uri_;
  std::string root_path_;
};

#endif  // STATIC_REQUEST_HANDLER_H