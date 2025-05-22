#ifndef BLOCKING_HANDLER_H
#define BLOCKING_HANDLER_H

#include "config_parser.h"
#include "request_handler.h"

#define DEFAULT_SLEEP_DURATION_SECONDS 3

class BlockingRequestHandlerArgs : public RequestHandlerArgs {
 public:
  BlockingRequestHandlerArgs();
  static std::shared_ptr<BlockingRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
};

class BlockingRequestHandlerTest;

class BlockingRequestHandler : public RequestHandler {
 public:
  BlockingRequestHandler(const std::string& uri,
                         std::shared_ptr<BlockingRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request& request) override;
  RequestHandler::HandlerType get_type() const override;
  friend class BlockingRequestHandlerTest;
};

#endif
