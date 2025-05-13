#ifndef REGISTRY_H
#define REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "config_parser.h"
#include "request_handler.h"

// Factory function signature: takes parsed args, returns a unique_ptr to a
// RequestHandler
using RequestHandlerFactory = std::function<std::unique_ptr<RequestHandler>(
    const std::string&, const std::string&)>;

// Check location function signature: takes config statement and location,
// returns bool
using CheckLocationFactory =
    std::function<bool(std::shared_ptr<NginxConfigStatement>, NginxLocation&)>;

// Macro to simplify registration of handlers with two-string constructor
// signature ex) Register the StaticRequestHandler under its config name
// REGISTER_HANDLER(StaticRequestHandler::kName, StaticRequestHandler)
#define REGISTER_HANDLER(NAME, HANDLER_NAME)                                   \
  static const bool _##HANDLER_NAME##_registered = Registry::register_handler( \
      NAME,                                                                    \
      [](const std::string& a,                                                 \
         const std::string& b) -> std::unique_ptr<RequestHandler> {            \
        return std::make_unique<HANDLER_NAME>(a, b);                           \
      },                                                                       \
      [](std::shared_ptr<NginxConfigStatement> s, NginxLocation& l) -> bool {  \
        return HANDLER_NAME::check_location(s, l);                             \
      })

// Registry for RequestHandler factories
class Registry {
 public:
  // Register a handler factory under a given name
  static bool register_handler(const std::string& name,
                               RequestHandlerFactory factory,
                               CheckLocationFactory check_location);

  static std::shared_ptr<RequestHandlerFactory> get_handler_factory(
      const std::string& name);

  static std::shared_ptr<CheckLocationFactory> get_check_location(
      const std::string& name);

  // Using Meyers' singleton pattern to ensure that the map is created only once
  // This prevents multiple instances of the map from being created in gtest
  static std::unordered_map<std::string, RequestHandlerFactory>&
  get_factory_map();

  static std::unordered_map<std::string, CheckLocationFactory>&
  get_check_location_map();
};

#endif  // REGISTRY_H