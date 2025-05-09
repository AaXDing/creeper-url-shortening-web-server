#include "registry.h"

#include "logging.h" 
// TODO: logging to be added


bool Registry::register_handler(const std::string& name, RequestHandlerFactory factory) {
  get_factory_map()[name] = std::move(factory);
  LOG(debug) << "Registry::register_handler: name=" << name;
  return true;
}

// create_handler implementation
std::unique_ptr<RequestHandler> Registry::create_handler(RequestHandlerFactory factory,
                                                         const std::string& arg1,
                                                         const std::string& arg2) {
  return factory(arg1, arg2);
}

std::shared_ptr<RequestHandlerFactory> Registry::get_handler_factory(const std::string& name) {
  LOG(debug) << "Registry::get_handler_factory: name=" << name;
  auto it = get_factory_map().find(name);
  if (it == get_factory_map().end()) {
      return nullptr;
  }
  return std::make_shared<RequestHandlerFactory>(it->second);
}

// Using Meyers' singleton pattern to ensure that the map is created only once
// This prevents multiple instances of the map from being created in gtest
std::unordered_map<std::string, RequestHandlerFactory>& Registry::get_factory_map() {
  // this map is stored in BSS section and visible to all classes
  static std::unordered_map<std::string, RequestHandlerFactory> factory_map_;
  return factory_map_;
}