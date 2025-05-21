#include "request_handler_dispatcher.h"

#include <string>

#include "config_parser.h"
#include "echo_request_handler.h"
#include "logging.h"
#include "registry.h"
#include "request_handler.h"
#include "static_request_handler.h"

RequestHandlerDispatcher::RequestHandlerDispatcher(const NginxConfig& config) {
  if (!add_routes(config)) {
    LOG(error) << "Failed to add handlers to dispatcher";
    throw std::runtime_error("Failed to add handlers to dispatcher");
  }
}

RequestHandlerDispatcher::~RequestHandlerDispatcher() {}

bool RequestHandlerDispatcher::add_routes(const NginxConfig& config) {
  NginxLocationResult result = config.get_locations();
  if (!result.valid) {
    LOG(error) << "Invalid locations in config";
    return false;
  }
  std::vector<NginxLocation> locations = result.locations;
  for (const auto& location : locations) {
    if (!add_route(location)) {
      LOG(warning) << "Dispatcher failed to add route for URI "
                   << location.path;
    }
  }
  return true;
}

bool RequestHandlerDispatcher::add_route(const NginxLocation& location) {
  std::string uri = location.path;
  std::string handler_type = location.handler;

  // Check if the URI already exists in the map
  // Only the first handler for a URI is added
  if (routes_.find(uri) != routes_.end()) {
    LOG(warning) << "Handler for URI \"" << uri << "\" already exists";
    return false;  // URI already exists
  }

  // register the handler type with uri and root path
  RequestHandlerFactoryPtr factory_ptr =
      Registry::get_handler_factory(handler_type);

  routes_[uri] =
      std::make_shared<std::tuple<std::shared_ptr<RequestHandlerFactory>,
                                  std::string, std::string>>(
          std::make_tuple(factory_ptr, uri, location.root.value_or("")));
  return true;
}

std::unique_ptr<RequestHandler> RequestHandlerDispatcher::get_handler(
    const Request& req) {
  std::string url = req.uri;
  std::string location = longest_prefix_match(url);

  RequestHandlerFactoryAndWorkersPtr factory_and_workers_ptr =
      routes_[location];
  RequestHandlerFactoryPtr factory_ptr = std::get<0>(*factory_and_workers_ptr);
  std::string uri = std::get<1>(*factory_and_workers_ptr);
  std::string root = std::get<2>(*factory_and_workers_ptr);

  return (*factory_ptr)(uri, root);
}

std::string RequestHandlerDispatcher::longest_prefix_match(
    const std::string& url) {
  // First argument is the URL to match, second argument is the URI in config
  size_t max_length = 0;
  std::string longest_match = "";

  for (const auto& route : routes_) {
    const std::string& route_uri = route.first;
    if (url.substr(0, route_uri.size()) == route_uri) {
      if (route_uri.size() > max_length) {
        max_length = route_uri.size();
        longest_match = route_uri;
      }
    }
  }

  return longest_match;
}