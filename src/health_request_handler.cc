#include "health_request_handler.h"

#include "config_parser.h"
#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("HealthHandler", HealthRequestHandler);

HealthRequestHandler::HealthRequestHandler(const std::string& arg1,
                                           const std::string& arg2) {}

std::unique_ptr<Response> HealthRequestHandler::handle_request(
    const Request& req) {
  auto res = std::make_unique<Response>();

  // Always return 200 OK with "OK" as the response body
  res->status_code = 200;
  res->status_message = "OK";
  res->version = req.valid ? req.version : HTTP_VERSION;
  res->content_type = "text/plain";
  res->body = "OK";

  LOG(info) << "Health check request handled successfully";
  return res;
}

bool HealthRequestHandler::check_location(
    std::shared_ptr<NginxConfigStatement> statement, NginxLocation& location) {
  // Health handler doesn't need any configuration
  return true;
}

RequestHandler::HandlerType HealthRequestHandler::get_type() const {
  return RequestHandler::HandlerType::HEALTH_REQUEST_HANDLER;
}