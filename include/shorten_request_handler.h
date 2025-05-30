#ifndef SHORTEN_REQUEST_HANDLER_H
#define SHORTEN_REQUEST_HANDLER_H

#include "config_parser.h"
#include "request_handler.h"

class ShortenRequestHandlerArgs : public RequestHandlerArgs {
 public:
  ShortenRequestHandlerArgs();
  static std::shared_ptr<ShortenRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
};

class ShortenRequestHandler : public RequestHandler {
 public:
  ShortenRequestHandler(const std::string& base_uri,
                        std::shared_ptr<ShortenRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request& request) override;
  RequestHandler::HandlerType get_type() const override;
  std::unique_ptr<Response> handle_post_request(const Request& request);
  std::unique_ptr<Response> handle_get_request(const Request& request);

 private:
  std::string shorten_url(const std::string& url);
  std::string base62_encode(const std::string& url);
  static std::unordered_map<std::string, std::string>& get_short_to_long_map();
  const std::string BASE62_CHARS =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::string base_uri_;
};

#endif  // SHORTEN_REQUEST_HANDLER_H