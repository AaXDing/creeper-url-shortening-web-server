#ifndef ECHO_REQUEST_HANDLER_H
#define ECHO_REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"

class EchoRequestHandlerArgs : public RequestHandlerArgs {
 public:
  EchoRequestHandlerArgs();
  static std::shared_ptr<EchoRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
};

class EchoRequestHandlerTest;  // forward declaration for test fixture

class EchoRequestHandler : public RequestHandler {
  /*
      This class handles HTTP requests by echoing back the request message.
      It inherits from RequestHandler and implements the handle_request method.
  */
 public:
  EchoRequestHandler(std::string base_uri, std::shared_ptr<EchoRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request& req) override;
  static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                             NginxLocation& location);
  RequestHandler::HandlerType get_type() const override;
  friend class EchoRequestHandlerTest;
};

#endif  // ECHO_REQUEST_HANDLER_H