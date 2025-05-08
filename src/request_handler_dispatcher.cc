#include "request_handler_dispatcher.h"

#include <string>

#include "config_parser.h"
#include "echo_request_handler.h"
#include "logging.h"
#include "request_handler.h"
#include "static_request_handler.h"

// TODO: discuss with team about how to handle errors
RequestHandlerDispatcher::RequestHandlerDispatcher(const NginxConfig& config) {
  if (!add_handlers(config)) {
    LOG(error) << "Failed to add handlers to dispatcher";
    throw std::runtime_error("Failed to add handlers to dispatcher");
  }
}

RequestHandlerDispatcher::~RequestHandlerDispatcher() {}

size_t RequestHandlerDispatcher::get_num_handlers() { return handlers_.size(); }

bool RequestHandlerDispatcher::add_handlers(const NginxConfig& config) {
  NginxLocationResult result = config.get_locations();
  if (!result.valid) {
    LOG(error) << "Invalid locations in config";
    return false;
  }
  std::vector<NginxLocation> locations = result.locations;
  for (const auto& location : locations) {
    if (!add_handler(location)) {
      LOG(warning) << "Dispatcher failed to add handler for URI "
                   << location.path;
    }
  }
  return true;
}

bool RequestHandlerDispatcher::add_handler(NginxLocation location) {
  std::string uri = location.path;
  while (uri.size() > 1 && uri[uri.size() - 1] == '/') {
    uri.pop_back();  // Remove trailing slashes
    LOG(debug) << "Dispatcher trimmed URI to=" << uri;
  }

  // Check if the URI already exists in the map
  // Only the first handler for a URI is added
  if (handlers_.find(uri) != handlers_.end()) {
    LOG(warning) << "Handler for URI \"" << uri << "\" already exists";
    return false;  // URI already exists
  }

  std::string handler_type;

  handler_type = location.handler;

  if (handler_type == "EchoHandler") {
    handlers_[uri] = std::make_shared<EchoRequestHandler>();
    LOG(info) << "Added EchoRequestHandler for URI \"" << uri << "\"";
    return true;
  }

  if (handler_type == "StaticHandler") {
    std::string root_path = location.root.value();

    auto handler = StaticRequestHandler::create(uri, root_path);

    if (handler == nullptr) {
      LOG(error) << "Failed to create StaticRequestHandler for URI \"" << uri
                 << "\"";
      return false;  // Failed to create handler
    }

    handlers_[uri] = std::shared_ptr<RequestHandler>(handler);
    LOG(info) << "Added StaticRequestHandler for URI \"" << uri
              << "\" with root path \"" << root_path << "\"";
    return true;
  }

  return false;
}

std::shared_ptr<RequestHandler> RequestHandlerDispatcher::get_handler(
    const Request& req) const {
  std::string uri = req.uri;
  // Remove trailing slashes from the URI
  while (uri.size() > 0 && uri[uri.size() - 1] == '/') {
    uri.pop_back();  // Remove trailing slashes
    LOG(debug) << "Dispatcher trimmed request URI to=" << uri;
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