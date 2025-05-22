#ifndef STATIC_REQUEST_HANDLER_H
#define STATIC_REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"
class StaticRequestHandlerTest;  // forward declaration for test fixture

class StaticRequestHandlerArgs : public RequestHandlerArgs {
 public:
  StaticRequestHandlerArgs(std::string root_path);
  static std::shared_ptr<StaticRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
  std::string get_root_path() const;

 private:
  std::string root_path_;
};
class StaticRequestHandler : public RequestHandler {
 public:
  StaticRequestHandler(std::string base_uri,
                       std::shared_ptr<StaticRequestHandlerArgs> args);
  // Handle the Request and return Response
  std::unique_ptr<Response> handle_request(const Request& req) override;
  RequestHandler::HandlerType get_type() const override;

  friend class StaticRequestHandlerTest;

 private:
  std::string generate_file_path(const std::string& file_path) const;
  std::string get_file_content_type(const std::string& file_path) const;
  std::string base_uri_;
  std::string root_path_;
};

#endif  // STATIC_REQUEST_HANDLER_H