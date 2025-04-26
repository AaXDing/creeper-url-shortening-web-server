#include "request_handler_dispatcher.h"

#include <string>

#include "config_parser.h"
#include "echo_request_handler.h"
#include "logging.h"
#include "request_handler.h"
#include "static_request_handler.h"

namespace {
// Factory map for URI to handler creation
const std::unordered_map<std::string,
                         std::function<std::shared_ptr<RequestHandler>()>>
    kHandlerFactory = {
        {"/echo", []() { return std::make_shared<EchoRequestHandler>(); }},
        {"/static", []() { return std::make_shared<StaticRequestHandler>(); }},
};
}  // namespace

RequestHandlerDispatcher::RequestHandlerDispatcher(const NginxConfig& config) {
  add_handlers(config);
}

RequestHandlerDispatcher::~RequestHandlerDispatcher() {}

size_t RequestHandlerDispatcher::get_num_handlers() { return handlers_.size(); }

void RequestHandlerDispatcher::add_handlers(const NginxConfig& config) {
  // Iterate through the config blocks and initialize handlers
  for (const auto& statement : config.statements_) {
    if (statement->tokens_.size() == 1 && statement->tokens_[0] == "server") {
      for (const auto& child_statement : statement->child_block_->statements_) {
        if (child_statement->tokens_.size() == 2 &&
            child_statement->tokens_[0] == "location") {
          if (!add_handler(child_statement->tokens_[1],
                           child_statement->child_block_)) {
            LOG(warning) << "Failed to add URI: "
                         << child_statement->tokens_[1];
          } else {
            LOG(info) << "Added URI: " << child_statement->tokens_[1];
          }
        }
      }
      break;
    }
  }
}

bool RequestHandlerDispatcher::add_handler(
    std::string uri, std::unique_ptr<NginxConfig>& config) {
  while (uri.size() > 1 && uri[uri.size() - 1] == '/') {
    uri.pop_back();  // Remove trailing slashes
  }

  // Check if the URI already exists in the map
  if (handlers_.find(uri) != handlers_.end()) {
    return false;  // URI already exists
  }

  if (config == nullptr) {
    return false;  // Invalid config
  }

  // Look up a handler creator in the factory map
  auto factory_it = kHandlerFactory.find(uri);
  if (factory_it != kHandlerFactory.end()) {
    handlers_[uri] = factory_it->second();
    return true;
  }
  return false;  // No handler created for this URI
}

std::shared_ptr<RequestHandler> RequestHandlerDispatcher::get_handler(
    const Request& req) const {
  std::string uri = req.uri;
  // Remove trailing slashes from the URI
  while (uri.size() > 0 && uri[uri.size() - 1] == '/') {
    uri.pop_back();  // Remove trailing slashes
  }

  std::shared_ptr<RequestHandler> handler = nullptr;
  size_t max_length = 0;

  // longest prefix match
  for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
    if (uri.substr(0, it->first.size()).compare(it->first) == 0) {
      if (it->first.size() > max_length) {
        max_length = it->first.size();
        handler = it->second;
      }
    }
  }

  if (handler == nullptr) {
    LOG(warning) << "No handler found for URI: " << uri;
  } else {
    LOG(info) << "Found handler for URI: " << uri;
  }
  return handler;
}