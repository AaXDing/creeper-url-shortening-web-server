#ifndef REGISTRY_H
#define REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "config_parser.h"
#include "request_handler.h"

// Factory function signature: takes base_uri and args, returns a unique_ptr to
// a RequestHandler
using RequestHandlerFactory = std::function<std::unique_ptr<RequestHandler>(
    const std::string&, std::shared_ptr<RequestHandlerArgs>)>;

// Check location function signature: takes config statement and location,
// returns bool
using CreateFromConfigFactory =
    std::function<std::shared_ptr<RequestHandlerArgs>(
        std::shared_ptr<NginxConfigStatement>)>;

// Macro to simplify registration of handlers with two-string constructor
// signature ex) Register the StaticRequestHandler under its config name
// REGISTER_HANDLER(StaticRequestHandler::kName, StaticRequestHandler,
// StaticRequestHandlerArgs)
#define REGISTER_HANDLER(NAME, HANDLER_NAME, ARGS_NAME)                        \
  static const bool _##HANDLER_NAME##_registered = Registry::register_handler( \
      NAME,                                                                    \
      [](const std::string& base_uri,                                          \
         std::shared_ptr<RequestHandlerArgs> args)                             \
          -> std::unique_ptr<RequestHandler> {                                 \
        return std::make_unique<HANDLER_NAME>(                                 \
            base_uri, std::static_pointer_cast<ARGS_NAME>(args));              \
      },                                                                       \
      [](std::shared_ptr<NginxConfigStatement> s)                              \
          -> std::shared_ptr<RequestHandlerArgs> {                             \
        return ARGS_NAME::create_from_config(s);                               \
      })

// Registry for RequestHandler factories
class Registry {
 public:
  // Register a handler factory under a given name
  static bool register_handler(const std::string& name,
                               RequestHandlerFactory factory,
                               CreateFromConfigFactory create_from_config);

  static std::shared_ptr<RequestHandlerFactory> get_handler_factory(
      const std::string& name);

  static std::shared_ptr<CreateFromConfigFactory> get_create_from_config(
      const std::string& name);

  // Using Meyers' singleton pattern to ensure that the map is created only once
  // This prevents multiple instances of the map from being created in gtest
  static std::unordered_map<std::string, RequestHandlerFactory>&
  get_factory_map();

  static std::unordered_map<std::string, CreateFromConfigFactory>&
  get_create_from_config_map();
};

#endif  // REGISTRY_H