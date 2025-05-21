#ifndef HEALTH_REQUEST_HANDLER_H
#define HEALTH_REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"

class HealthRequestHandler : public RequestHandler {
 public:
  HealthRequestHandler(const std::string& arg1, const std::string& arg2);
  std::unique_ptr<Response> handle_request(const Request& req) override;
  static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                           NginxLocation& location);
  RequestHandler::HandlerType get_type() const override;
};

#endif  // HEALTH_REQUEST_HANDLER_H 