#include "not_found_request_handler.h"

#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("NotFoundHandler", NotFoundRequestHandler,
                 NotFoundRequestHandlerArgs);

NotFoundRequestHandlerArgs::NotFoundRequestHandlerArgs() {}

std::shared_ptr<NotFoundRequestHandlerArgs>
NotFoundRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  if (statement->child_block_->statements_.size() != 0) {
    LOG(error) << "NotFoundHandler must have no arguments";
    return nullptr;
  }
  return std::make_shared<NotFoundRequestHandlerArgs>();
}

NotFoundRequestHandler::NotFoundRequestHandler(
    std::string base_uri, std::shared_ptr<NotFoundRequestHandlerArgs> args) {}

std::unique_ptr<Response> NotFoundRequestHandler::handle_request(
    const Request& req) {
  auto res = std::make_unique<Response>();

  LOG(info) << "Handling 404 Not Found request for URI: " << req.uri;

  // Set response properties
  res->version = req.valid ? req.version : HTTP_VERSION;
  res->status_code = 404;
  res->status_message = "Not Found";
  res->content_type = "text/plain";
  res->body = "404 Not Found";

  return res;
}

RequestHandler::HandlerType NotFoundRequestHandler::get_type() const {
  return RequestHandler::HandlerType::NOT_FOUND_REQUEST_HANDLER;
}