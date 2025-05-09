#ifndef REGISTRY_H
#define REGISTRY_H

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include "request_handler.h"

// Factory function signature: takes parsed args, returns a unique_ptr to a RequestHandler
using HandlerFactory = std::function<std::unique_ptr<RequestHandler>(const std::string&, const std::string&)>;

// Macro to simplify registration of handlers with two-string constructor signature
// ex) Register the StaticRequestHandler under its config name
// REGISTER_HANDLER(StaticRequestHandler::kName, StaticRequestHandler)
#define REGISTER_HANDLER(NAME, HandlerType)                      \
  static bool _##HandlerType##__registered =                     \
    Registry::register_handler(NAME,                             \
      [](const std::string& a, const std::string& b)             \
        -> std::unique_ptr<RequestHandler> {                     \
        return std::make_unique<HandlerType>(a, b);              \
      });

// Registry for RequestHandler factories
class Registry {
 public:
  // Register a handler factory under a given name
  static bool register_handler(const std::string& name, HandlerFactory factory);

  // Create a handler by name, passing along constructor args
  // Returns nullptr if no such handler registered
  static std::unique_ptr<RequestHandler> create_handler(const std::string& name,
                                                        const std::string& arg1,
                                                        const std::string& arg2);

 private:
  // Internal map from handler name to factory
  static std::unordered_map<std::string, HandlerFactory> _map;
};

#endif // REGISTRY_H