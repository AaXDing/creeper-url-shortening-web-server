#ifndef SHORTEN_REQUEST_HANDLER_H
#define SHORTEN_REQUEST_HANDLER_H

#include "config_parser.h"
#include "idatabase_client.h"
#include "iredis_client.h"
#include "request_handler.h"

class ShortenRequestHandlerArgs : public RequestHandlerArgs {
 public:
  ShortenRequestHandlerArgs();
  static std::shared_ptr<ShortenRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
  std::shared_ptr<IRedisClient> redis_client;
  std::shared_ptr<IDatabaseClient> db_client;
};

class ShortenRequestHandler : public RequestHandler {
 public:
  ShortenRequestHandler(const std::string& base_uri,
                        std::shared_ptr<ShortenRequestHandlerArgs> args);
  ~ShortenRequestHandler();
  std::unique_ptr<Response> handle_request(const Request& request) override;
  RequestHandler::HandlerType get_type() const override;
  std::unique_ptr<Response> handle_post_request(const Request& request);
  std::unique_ptr<Response> handle_get_request(const Request& request);

 private:
  static constexpr int SHORT_URL_LENGTH = 6;
  static constexpr char BASE62_CHARS[] =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::string base_uri_;
  std::shared_ptr<IRedisClient> redis_;
  std::shared_ptr<IDatabaseClient> db_;
  std::string base62_encode(const std::string& url);
};

#endif  // SHORTEN_REQUEST_HANDLER_H