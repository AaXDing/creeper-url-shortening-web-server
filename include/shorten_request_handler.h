#ifndef SHORTEN_REQUEST_HANDLER_H
#define SHORTEN_REQUEST_HANDLER_H

#include "request_handler.h"
#include "config_parser.h"

class ShortenRequestHandlerArgs : public RequestHandlerArgs {
 public:
  ShortenRequestHandlerArgs();
  static std::shared_ptr<ShortenRequestHandlerArgs> create_from_config(std::shared_ptr<NginxConfigStatement> statement);
};

class ShortenRequestHandler : public RequestHandler {
 public:
  ShortenRequestHandler(const std::string& url, std::shared_ptr<ShortenRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request& request) override;
  RequestHandler::HandlerType get_type() const override;

 private:
  std::string shorten_url(const std::string& url);
  std::string base62_encode(const std::string& url);
  static std::unordered_map<std::string, std::string>& get_url_map();
  const std::string base62_chars_ = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
};

#endif  // SHORTEN_REQUEST_HANDLER_H