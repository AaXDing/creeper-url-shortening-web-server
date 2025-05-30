#include "blocking_request_handler.h"

#include <chrono>
#include <thread>

#include "config_parser.h"
#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("BlockingHandler", BlockingRequestHandler,
                 BlockingRequestHandlerArgs);

BlockingRequestHandlerArgs::BlockingRequestHandlerArgs() {}

std::shared_ptr<BlockingRequestHandlerArgs>
BlockingRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  return std::make_shared<BlockingRequestHandlerArgs>();
}

BlockingRequestHandler::BlockingRequestHandler(
    const std::string& uri, std::shared_ptr<BlockingRequestHandlerArgs> args) {}

std::unique_ptr<Response> BlockingRequestHandler::handle_request(
    const Request& request) {
  // Sleep for the specified duration
  std::this_thread::sleep_for(
      std::chrono::seconds(DEFAULT_SLEEP_DURATION_SECONDS));

  auto res = std::make_unique<Response>();
  res->status_code = 200;
  res->status_message = "OK";
  res->version = request.version;
  res->headers = {{"Content-Type", "text/plain"}};
  res->body = "Blocking request completed";
  return res;
}

RequestHandler::HandlerType BlockingRequestHandler::get_type() const {
  return RequestHandler::HandlerType::BLOCKING_REQUEST_HANDLER;
}