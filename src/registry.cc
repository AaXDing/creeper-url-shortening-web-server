#include "registry.h"

#include "logging.h" 
// TODO: logging to be added

// Define the static map storage
std::unordered_map<std::string, HandlerFactory> Registry::_map;

// register_handler implementation
bool Registry::register_handler(const std::string& name, HandlerFactory factory) {
  _map[name] = std::move(factory);
  return true;
}

// create_handler implementation
std::unique_ptr<RequestHandler> Registry::create_handler(const std::string& name,
                                                         const std::string& arg1,
                                                         const std::string& arg2) {
  auto it = _map.find(name);
  if (it == _map.end()) {
      return nullptr;
  }
  return it->second(arg1, arg2);
}