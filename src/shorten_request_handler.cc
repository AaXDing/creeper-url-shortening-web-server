#include "shorten_request_handler.h"

#include "logging.h"
#include "registry.h"

REGISTER_HANDLER("ShortenHandler", ShortenRequestHandler,
                 ShortenRequestHandlerArgs);

ShortenRequestHandlerArgs::ShortenRequestHandlerArgs() {}

std::shared_ptr<ShortenRequestHandlerArgs>
ShortenRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  return std::make_shared<ShortenRequestHandlerArgs>();
}

ShortenRequestHandler::ShortenRequestHandler(
    const std::string& base_uri,
    std::shared_ptr<ShortenRequestHandlerArgs> args)
    : base_uri_(base_uri) {}

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
  res->status_code = 405;
  res->status_message = "Method Not Allowed";
  res->version = request.version;
  res->content_type = "text/plain";
  res->body = "Method Not Allowed";
  return res;
}

// Long URL -> Short URL
std::unique_ptr<Response> ShortenRequestHandler::handle_post_request(
    const Request& request) {
  auto res = std::make_unique<Response>();
  res->status_code = 200;
  res->status_message = "OK";
  res->version = request.version;
  res->content_type = "text/plain";
  
  std::string short_url = shorten_url(request.body);
  if (short_url.empty()) {
    res->status_code = 400;
    res->status_message = "Bad Request";
    res->body = "Short URL already exist for another long URL";
    return res;
  }

  res->body = short_url;
  return res;
}

// Short URL -> Long URL
std::unique_ptr<Response> ShortenRequestHandler::handle_get_request(
    const Request& request) {
  auto res = std::make_unique<Response>();
  auto& short_to_long_map = get_short_to_long_map();

  // /base_uri/6UQVxS --> 6UQVxS
  std::string short_url = request.uri.substr(base_uri_.length() + 1);

  if (short_to_long_map.find(short_url) == short_to_long_map.end()) {
    res->status_code = 404;
    res->status_message = "Not Found";
    res->version = request.version;
    res->content_type = "text/plain";
    res->body = "Not Found";
    return res;
  }
  res->status_code = 302;
  res->status_message = "Found";
  res->version = request.version;
  res->content_type = "text/plain";
  res->body = short_to_long_map[short_url];

  return res;
}

std::string ShortenRequestHandler::shorten_url(const std::string& url) {
  auto& short_to_long_map = get_short_to_long_map();

  std::string encoded_url = base62_encode(url);

  if (short_to_long_map.find(encoded_url) == short_to_long_map.end()) {
    short_to_long_map[encoded_url] = url;
    return encoded_url;
  }

  if (url.compare(short_to_long_map[encoded_url]) != 0) {
    return encoded_url;
  }

  // TODO: We can do something else such as generating a new short URL
  // Now if Long URL collides, we return an empty string and return 400
  return "";
}

std::string ShortenRequestHandler::base62_encode(const std::string& url) {
  std::string base62_url;
  std::hash<std::string> hasher;
  size_t num = hasher(url);
  while (num > 0) {
    base62_url += BASE62_CHARS[num % 62];
    num /= 62;
  }
  
  // Restrict to 6 characters
  // If the encoded URL is less than 6 characters, pad with leading zeros
  while (base62_url.length() < 6) {
    base62_url = "0" + base62_url;
  }
  // If the encoded URL is greater than 6 characters, truncate to 6 characters
  base62_url = base62_url.substr(0, 6);

  return base62_url;
}

// use Meyers' singleton pattern
// Short URL -> Long URL map
std::unordered_map<std::string, std::string>&
ShortenRequestHandler::get_short_to_long_map() {
  static std::unordered_map<std::string, std::string> short_to_long_map;
  return short_to_long_map;
}