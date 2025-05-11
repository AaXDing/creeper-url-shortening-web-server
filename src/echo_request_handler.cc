// A class that handles HTTP requests

#include "echo_request_handler.h"

#include "config_parser.h"
#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("EchoHandler", EchoRequestHandler);

// dummy constructor
EchoRequestHandler::EchoRequestHandler(const std::string& arg1,
                                       const std::string& arg2) {}

std::unique_ptr<Response> EchoRequestHandler::handle_request(
    const Request& req) {
  auto res = std::make_unique<Response>();  // Create a new Response object

  if (req.valid) {  // If the request is valid, echo the request
    LOG(info) << "Valid echo request: " << req.method << " " << req.uri;
    res->status_code = 200;
    res->status_message = "OK";
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->content_type = "text/plain";
    res->body = req.to_string();  // Convert the request to a string
  } else {  // If the request is invalid, return a 400 Bad Request response
    LOG(warning) << "Invalid echo request → returning 400 Bad Request";
    *res = STOCK_RESPONSE.at(400);  // Return a 400 Bad Request response
  }
  LOG(trace) << "handle_request completed with status=" << res->status_code;
  return res;  // Return the response
}

bool EchoRequestHandler::check_location(
    std::shared_ptr<NginxConfigStatement> statement, NginxLocation& location) {
  if (statement->child_block_->statements_.size() != 0) {
    LOG(error) << "EchoHandler must have no arguments";
    return false;
  }
  return true;
}

RequestHandler::HandlerType EchoRequestHandler::get_type() const {
  return RequestHandler::HandlerType::ECHO_REQUEST_HANDLER;
}