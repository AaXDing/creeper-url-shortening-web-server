#ifndef SHORTEN_REQUEST_HANDLER_H
#define SHORTEN_REQUEST_HANDLER_H

#include <libpq-fe.h>
#include <sw/redis++/redis++.h>

#include "config_parser.h"
#include "request_handler.h"

using namespace sw::redis;

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
  ~ShortenRequestHandler();
  std::unique_ptr<Response> handle_request(const Request& request) override;
  RequestHandler::HandlerType get_type() const override;
  std::unique_ptr<Response> handle_post_request(const Request& request);
  std::unique_ptr<Response> handle_get_request(const Request& request);

 private:
  std::optional<std::string> shorten_url(const std::string& url);
  std::string base62_encode(const std::string& url);
  bool init_db();
  bool store_url_mapping(const std::string& short_url,
                         const std::string& long_url);
  std::optional<std::string> get_long_url(const std::string& short_url);
  const std::string BASE62_CHARS =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::string base_uri_;
  std::shared_ptr<Redis> redis_;
  PGconn* pg_conn_;
  const std::string REDIS_IP = "127.0.0.1";
  const int REDIS_PORT = 6379;
  const int SHORT_URL_LENGTH = 6;
  const std::string DB_HOST = "10.90.80.3";
  //   const std::string DB_HOST = "34.168.12.115";
  const std::string DB_NAME = "url-mapping";
  const std::string DB_USER = "creeper-server";
  const std::string DB_PASSWORD = "creeper";
};

#endif  // SHORTEN_REQUEST_HANDLER_H