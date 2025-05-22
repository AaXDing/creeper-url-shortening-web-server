#include "registry.h"

#include "logging.h"

bool Registry::register_handler(const std::string& name,
                                RequestHandlerFactory factory,
                                CreateFromConfigFactory create_from_config) {
  get_factory_map()[name] = std::move(factory);
  get_create_from_config_map()[name] = std::move(create_from_config);
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

std::shared_ptr<CreateFromConfigFactory> Registry::get_create_from_config(
    const std::string& name) {
  LOG(debug) << "Registry::get_create_from_config: name=" << name;
  auto it = get_create_from_config_map().find(name);
  if (it == get_create_from_config_map().end()) {
    return nullptr;
  }
  return std::make_shared<CreateFromConfigFactory>(it->second);
}

// Using Meyers' singleton pattern to ensure that the map is created only once
// This prevents multiple instances of the map from being created in gtest
std::unordered_map<std::string, RequestHandlerFactory>&
Registry::get_factory_map() {
  // this map is stored in BSS section and visible to all classes
  static std::unordered_map<std::string, RequestHandlerFactory> factory_map_;
  return factory_map_;
}

std::unordered_map<std::string, CreateFromConfigFactory>&
Registry::get_create_from_config_map() {
  // this map is stored in BSS section and visible to all classes
  static std::unordered_map<std::string, CreateFromConfigFactory>
      create_from_config_map_;
  return create_from_config_map_;
}