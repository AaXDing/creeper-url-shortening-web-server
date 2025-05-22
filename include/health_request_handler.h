#ifndef HEALTH_REQUEST_HANDLER_H
#define HEALTH_REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"

class HealthRequestHandlerArgs : public RequestHandlerArgs {
 public:
  HealthRequestHandlerArgs();
  static std::shared_ptr<HealthRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
};

class HealthRequestHandler : public RequestHandler {
 public:
  HealthRequestHandler(std::string base_uri, std::shared_ptr<HealthRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request& req) override;
  static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                             NginxLocation& location);
  RequestHandler::HandlerType get_type() const override;
};

#endif  // HEALTH_REQUEST_HANDLER_H