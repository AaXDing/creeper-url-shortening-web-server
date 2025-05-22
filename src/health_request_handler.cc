#include "health_request_handler.h"

#include "config_parser.h"
#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("HealthHandler", HealthRequestHandler,
                 HealthRequestHandlerArgs);

HealthRequestHandlerArgs::HealthRequestHandlerArgs() {}

std::shared_ptr<HealthRequestHandlerArgs>
HealthRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  if (statement->child_block_->statements_.size() != 0) {
    LOG(error) << "HealthHandler must have no arguments";
    return nullptr;
  }
  return std::make_shared<HealthRequestHandlerArgs>();
}

HealthRequestHandler::HealthRequestHandler(
    std::string base_uri, std::shared_ptr<HealthRequestHandlerArgs> args) {}

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

RequestHandler::HandlerType HealthRequestHandler::get_type() const {
  return RequestHandler::HandlerType::HEALTH_REQUEST_HANDLER;
}