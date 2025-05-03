#include "request_handler_dispatcher.h"

#include <string>

#include "config_parser.h"
#include "echo_request_handler.h"
#include "logging.h"
#include "request_handler.h"
#include "static_request_handler.h"

RequestHandlerDispatcher::RequestHandlerDispatcher(const NginxConfig& config) {
  add_handlers(config);
}

RequestHandlerDispatcher::~RequestHandlerDispatcher() {}

size_t RequestHandlerDispatcher::get_num_handlers() { return handlers_.size(); }

void RequestHandlerDispatcher::add_handlers(const NginxConfig& config) {
  // Iterate through the config blocks and initialize handlers
  for (const auto& statement : config.statements_) {
    if (statement->tokens_.size() == 1 && statement->tokens_[0] == "server") {
      for (const auto& child_statement : statement->child_block_->statements_) {
        if (child_statement->tokens_.size() == 2 &&
            child_statement->tokens_[0] == "location") {
          if (!add_handler(child_statement->tokens_[1],
                           child_statement->child_block_)) {
          }
        }
      }
      break;
    }
  }
}

bool RequestHandlerDispatcher::add_handler(
    std::string uri, std::unique_ptr<NginxConfig>& config) {
  while (uri.size() > 1 && uri[uri.size() - 1] == '/') {
    uri.pop_back();  // Remove trailing slashes
  }

  // Check if the URI already exists in the map
  // Only the first handler for a URI is added
  if (handlers_.find(uri) != handlers_.end()) {
    LOG(warning) << "Handler for URI \"" << uri << "\" already exists";
    return false;  // URI already exists
  }

  if (config == nullptr) {
    LOG(error) << "Null config for URI \"" << uri << "\"";
    return false;  // Invalid config
  }

  std::string handler_type;

  if (config->statements_.size() >= 1) {
    // for echo handler
    if (config->statements_[0]->tokens_.size() == 2 &&
        config->statements_[0]->tokens_[0] == "handler") {
      handler_type = config->statements_[0]->tokens_[1];
      if (handler_type == "echo") {
        handlers_[uri] = std::make_shared<EchoRequestHandler>();
        LOG(info) << "Added EchoRequestHandler for URI \"" << uri << "\"";
        return true;
      } else if (handler_type == "static") {
        if (config->statements_.size() == 2 &&
            config->statements_[1]->tokens_.size() == 2 &&
            config->statements_[1]->tokens_[0] == "root") {
          std::string root_path = config->statements_[1]->tokens_[1];
          auto handler = StaticRequestHandler::create(uri, root_path);
          if (handler == nullptr) {
            LOG(error) << "Failed to create StaticRequestHandler for URI \""
                       << uri << "\"";
            return false;  // Failed to create handler
          }
          handlers_[uri] = std::shared_ptr<RequestHandler>(handler);
          LOG(info) << "Added StaticRequestHandler for URI \"" << uri
                    << "\" with root path \"" << root_path << "\"";
          return true;
        } else {
          LOG(error) << "Missing root path for static handler for URI \"" << uri
                     << "\"";
          return false;  // Missing root path
        }

      } else {
        LOG(error) << "Invalid handler type \"" << handler_type
                   << "\" for URI \"" << uri << "\"";
        return false;  // Invalid handler type
      }
    } else {
      LOG(error) << "Invalid config for URI \"" << uri << "\". "
                 << "Expected \"handler <type>\"";
      return false;  // Invalid config
    }
  } else {
    LOG(error) << "Invalid config for URI \"" << uri << "\"";
    return false;  // Invalid config
  }
  return false;  // No handler created for this URI
}

std::shared_ptr<RequestHandler> RequestHandlerDispatcher::get_handler(
    const Request& req) const {
  std::string uri = req.uri;
  // Remove trailing slashes from the URI
  while (uri.size() > 0 && uri[uri.size() - 1] == '/') {
    uri.pop_back();  // Remove trailing slashes
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