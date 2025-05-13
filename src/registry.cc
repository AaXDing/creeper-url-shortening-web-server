#include "registry.h"

#include "logging.h"
// TODO: logging to be added

bool Registry::register_handler(const std::string& name,
                                RequestHandlerFactory factory,
                                CheckLocationFactory check_location) {
  get_factory_map()[name] = std::move(factory);
  get_check_location_map()[name] = std::move(check_location);
  LOG(debug) << "Registry::register_handler: name=" << name;
  return true;
}

std::shared_ptr<RequestHandlerFactory> Registry::get_handler_factory(
    const std::string& name) {
  LOG(debug) << "Registry::get_handler_factory: name=" << name;
  auto it = get_factory_map().find(name);
  if (it == get_factory_map().end()) {
    return nullptr;
  }
  return std::make_shared<RequestHandlerFactory>(it->second);
}

std::shared_ptr<CheckLocationFactory> Registry::get_check_location(
    const std::string& name) {
  LOG(debug) << "Registry::get_check_location: name=" << name;
  auto it = get_check_location_map().find(name);
  if (it == get_check_location_map().end()) {
    return nullptr;
  }
  return std::make_shared<CheckLocationFactory>(it->second);
}

// Using Meyers' singleton pattern to ensure that the map is created only once
// This prevents multiple instances of the map from being created in gtest
std::unordered_map<std::string, RequestHandlerFactory>&
Registry::get_factory_map() {
  // this map is stored in BSS section and visible to all classes
  static std::unordered_map<std::string, RequestHandlerFactory> factory_map_;
  return factory_map_;
}

std::unordered_map<std::string, CheckLocationFactory>&
Registry::get_check_location_map() {
  // this map is stored in BSS section and visible to all classes
  static std::unordered_map<std::string, CheckLocationFactory>
      check_location_map_;
  return check_location_map_;
}