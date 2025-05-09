#ifndef REGISTRY_H
#define REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "request_handler.h"

// Factory function signature: takes parsed args, returns a unique_ptr to a
// RequestHandler
using RequestHandlerFactory = std::function<std::unique_ptr<RequestHandler>(
    const std::string&, const std::string&)>;

// Macro to simplify registration of handlers with two-string constructor
// signature ex) Register the StaticRequestHandler under its config name
// REGISTER_HANDLER(StaticRequestHandler::kName, StaticRequestHandler)
#define REGISTER_HANDLER(NAME, HANDLER_NAME)                                   \
  static const bool _##HANDLER_NAME##_registered = Registry::register_handler( \
      NAME,                                                                    \
      [](const std::string& a,                                                 \
         const std::string& b) -> std::unique_ptr<RequestHandler> {            \
        return std::make_unique<HANDLER_NAME>(a, b);                           \
      })
// Registry for RequestHandler factories
class Registry {
 public:
  // Register a handler factory under a given name
  static bool register_handler(const std::string& name,
                               RequestHandlerFactory factory);

  // Create a handler by name, passing along constructor args
  // Returns nullptr if no such handler registered
  static std::unique_ptr<RequestHandler> create_handler(
      RequestHandlerFactory factory, const std::string& arg1,
      const std::string& arg2);

  static std::shared_ptr<RequestHandlerFactory> get_handler_factory(
      const std::string& name);

  // Using Meyers' singleton pattern to ensure that the map is created only once
  // This prevents multiple instances of the map from being created in gtest
  static std::unordered_map<std::string, RequestHandlerFactory>&
  get_factory_map();
};

#endif  // REGISTRY_H