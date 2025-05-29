#include "shorten_request_handler.h"
#include "registry.h"
#include "logging.h"

REGISTER_HANDLER("ShortenHandler", ShortenRequestHandler, ShortenRequestHandlerArgs);

ShortenRequestHandlerArgs::ShortenRequestHandlerArgs() {}

std::shared_ptr<ShortenRequestHandlerArgs> ShortenRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
    return std::make_shared<ShortenRequestHandlerArgs>();
}

ShortenRequestHandler::ShortenRequestHandler(const std::string& url, std::shared_ptr<ShortenRequestHandlerArgs> args) {}

RequestHandler::HandlerType ShortenRequestHandler::get_type() const {
    return RequestHandler::HandlerType::SHORTEN_REQUEST_HANDLER;
}

std::unique_ptr<Response> ShortenRequestHandler::handle_request(const Request& request) {
    auto res = std::make_unique<Response>();
    res->status_code = 200;
    res->status_message = "OK";
    res->version = request.version;
    res->content_type = "text/plain"; 
    res->body = shorten_url(request.body);
    
    return res;
}

std::string ShortenRequestHandler::shorten_url(const std::string& url) {
    auto& url_map = get_url_map();

    std::string base62_url;
    if (url_map.find(url) == url_map.end()) {
        base62_url = base62_encode(url);
        url_map[url] = base62_url;
        return base62_url;
    }
    else {
        base62_url = url_map[url];
    }
    return base62_url;
}

std::string ShortenRequestHandler::base62_encode(const std::string& url) {
    std::string base62_url;
    std::hash<std::string> hasher;
    size_t num = hasher(url);
    while (num > 0) {
        base62_url += base62_chars_[num % 62];
        num /= 62;
    }
    return base62_url;
}

// use Meyers' singleton pattern
std::unordered_map<std::string, std::string>& ShortenRequestHandler::get_url_map() {
    static std::unordered_map<std::string, std::string> url_map;
    return url_map;
}