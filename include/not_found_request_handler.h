#ifndef NOT_FOUND_REQUEST_HANDLER_H
#define NOT_FOUND_REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"

class NotFoundRequestHandlerArgs : public RequestHandlerArgs {
 public:
  NotFoundRequestHandlerArgs();
  static std::shared_ptr<NotFoundRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
};

class NotFoundRequestHandler : public RequestHandler {
 public:
  NotFoundRequestHandler(std::string base_uri,
                         std::shared_ptr<NotFoundRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request& req) override;
  RequestHandler::HandlerType get_type() const override;
};

#endif  // NOT_FOUND_REQUEST_HANDLER_H