#include "shorten_request_handler.h"

#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("ShortenHandler", ShortenRequestHandler,
                 ShortenRequestHandlerArgs);

std::unordered_map<std::string, std::string>
    ShortenRequestHandler::short_to_long_map_;

ShortenRequestHandlerArgs::ShortenRequestHandlerArgs() {}

std::shared_ptr<ShortenRequestHandlerArgs>
ShortenRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  return std::make_shared<ShortenRequestHandlerArgs>();
}

ShortenRequestHandler::ShortenRequestHandler(
    const std::string& base_uri,
    std::shared_ptr<ShortenRequestHandlerArgs> args)
    : base_uri_(base_uri) {
  // Try to connect to Redis
  try {
    std::string redis_uri =
        "tcp://" + REDIS_IP + ":" + std::to_string(REDIS_PORT);
    redis_ = std::make_shared<Redis>(redis_uri);
  } catch (const Error& e) {
    LOG(fatal) << "Failed to connect to Redis: " << e.what();
    exit(1);
  }
}

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

  // If short URL is not in DB, store the (short URL, long URL) mapping
  if (short_to_long_map_.find(short_url) == short_to_long_map_.end()) {
    // Store the (short URL, long URL) mapping in DB
    short_to_long_map_[short_url] = long_url;
    LOG(info) << "Stored in DB: " << short_url << " -> " << long_url;
  }
  // If long URL exists and does not match the incoming long URL, return 400
  else if (long_url.compare(short_to_long_map_[short_url]) != 0) {
    LOG(info) << "Long URL already exists: " << short_url << " -> "
                 << short_to_long_map_[short_url];
    *res = STOCK_RESPONSE.at(400);
    return res;
  }

  // TODO: We can do something else such as generating a new short URL
  // Now if Long URL collides, we return an empty string and return 400
  // Future if the long URL is changed, we can update the short URL
  // Use Redis to delete the short URL and the long URL in cache
  // redis_->del(encoded_url);

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

  // If Short URL is not found in Redis, check if it's in the DB
  if (short_to_long_map_.find(short_url) == short_to_long_map_.end()) {
    LOG(info) << "Not Found in DB: " << short_url;
    *res = STOCK_RESPONSE.at(404);
    return res;
  }

  // If Short URL is found in DB
  LOG(info) << "Found in DB: " << short_url << " -> "
            << short_to_long_map_[short_url];

  // Store the short URL, long URL mapping in Redis
  redis_->set(short_url, short_to_long_map_[short_url]);

  res->status_code = 302;
  res->status_message = "Found";
  res->version = request.version;
  // Set the location header to the long URL for redirection
  res->headers.push_back({"Location", short_to_long_map_[short_url]});
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
