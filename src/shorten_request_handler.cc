#include "shorten_request_handler.h"

#include "logging.h"
#include "real_database_client.h"
#include "real_redis_client.h"
#include "registry.h"

REGISTER_HANDLER("ShortenHandler", ShortenRequestHandler,
                 ShortenRequestHandlerArgs);

ShortenRequestHandlerArgs::ShortenRequestHandlerArgs() {}

std::shared_ptr<ShortenRequestHandlerArgs>
ShortenRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  auto args = std::make_shared<ShortenRequestHandlerArgs>();

  const std::string redis_ip = "127.0.0.1";
  const int redis_port = 6379;
  const std::string db_host = "10.90.80.3";  // Private
  // const std::string db_host = "34.168.12.115";  // Public
  const std::string db_name = "url-mapping";
  const std::string db_user = "creeper-server";
  const std::string db_pass = "creeper";

  // Construct concrete implementations and store them in args:
  args->redis_client = std::make_shared<RealRedisClient>(redis_ip, redis_port);
  args->db_client =
      std::make_shared<RealDatabaseClient>(db_host, db_name, db_user, db_pass);

  return args;
}

ShortenRequestHandler::ShortenRequestHandler(
    const std::string& base_uri,
    std::shared_ptr<ShortenRequestHandlerArgs> args)
    : base_uri_(base_uri), redis_(args->redis_client), db_(args->db_client) {
  if (!redis_) {
    LOG(fatal) << "ShortenRequestHandler requires a valid IRedisClient";
    exit(1);
  }
  if (!db_) {
    LOG(fatal) << "ShortenRequestHandler requires a valid IDatabaseClient";
    exit(1);
  }
}

ShortenRequestHandler::~ShortenRequestHandler() = default;

RequestHandler::HandlerType ShortenRequestHandler::get_type() const {
  return RequestHandler::HandlerType::SHORTEN_REQUEST_HANDLER;
}

std::unique_ptr<Response> ShortenRequestHandler::handle_request(
    const Request& request) {
  if (request.method == "POST") {
    return handle_post_request(request);
  } else if (request.method == "GET") {
    return handle_get_request(request);
  }

  auto res = std::make_unique<Response>();
  *res = STOCK_RESPONSE.at(405);
  return res;
}

// Long URL -> Short URL
std::unique_ptr<Response> ShortenRequestHandler::handle_post_request(
    const Request& request) {
  auto res = std::make_unique<Response>();

  std::string long_url = request.body;
  std::string short_url = base62_encode(long_url);

  // Store the mapping in PostgreSQL
  if (!db_->store(short_url, long_url)) {
    LOG(error) << "Failed to store URL mapping: " << short_url << " -> "
               << long_url;
    *res = STOCK_RESPONSE.at(500);
    res->body = "Failed to store URL mapping";
    return res;
  }

  res->status_code = 200;
  res->status_message = "OK";
  res->version = request.version;
  res->headers.push_back({"Content-Type", "text/plain"});
  res->body = short_url;
  return res;
}

// Short URL -> Long URL
std::unique_ptr<Response> ShortenRequestHandler::handle_get_request(
    const Request& request) {
  auto res = std::make_unique<Response>();

  // Must be /base_uri/6UQVxS
  if (request.uri.length() != base_uri_.length() + SHORT_URL_LENGTH + 1) {
    LOG(info) << "Invalid short URL: " << request.uri
              << " (expected short URL length: " << SHORT_URL_LENGTH << ")";
    *res = STOCK_RESPONSE.at(404);
    return res;
  }

  // /base_uri/6UQVxS --> 6UQVxS
  std::string short_url = request.uri.substr(base_uri_.length() + 1);

  // If Short URL is found in Redis, return 302
  std::optional<std::string> redis_long_url = redis_->get(short_url);
  if (redis_long_url) {
    LOG(info) << "Found in Redis: " << short_url << " -> "
              << redis_long_url.value();
    res->status_code = 302;
    res->status_message = "Found";
    res->version = request.version;
    // Set the location header to the long URL for redirection
    res->headers.push_back({"Location", redis_long_url.value()});
    return res;
  }

  // If Short URL is not found in Redis, check SQL database
  // std::optional<std::string> long_url = get_long_url(short_url);
  std::optional<std::string> long_url = db_->lookup(short_url);
  if (!long_url) {
    LOG(info) << "Not Found in DB: " << short_url;
    *res = STOCK_RESPONSE.at(404);
    return res;
  }

  // If Short URL is found in SQL database
  LOG(info) << "Found in DB: " << short_url << " -> " << long_url.value();

  // Store the short URL, long URL mapping in Redis
  redis_->set(short_url, long_url.value());

  res->status_code = 302;
  res->status_message = "Found";
  res->version = request.version;
  // Set the location header to the long URL for redirection
  res->headers.push_back({"Location", long_url.value()});
  return res;
}

std::string ShortenRequestHandler::base62_encode(const std::string& url) {
  std::string base62_url;
  std::hash<std::string> hasher;
  size_t num = hasher(url);

  // Convert the number to base62
  while (num > 0) {
    base62_url += BASE62_CHARS[num % 62];
    num /= 62;
  }

  // Restrict to 6 characters
  // If the encoded URL is less than 6 characters, pad with leading zeros
  while (base62_url.length() < SHORT_URL_LENGTH) {
    base62_url = "0" + base62_url;
  }
  // If the encoded URL is greater than 6 characters, truncate to 6 characters
  base62_url = base62_url.substr(0, SHORT_URL_LENGTH);

  return base62_url;
}
