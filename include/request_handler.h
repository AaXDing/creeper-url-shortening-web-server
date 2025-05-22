#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <memory>
#include <string>

#include "http_header.h"

class RequestHandlerArgs {
 public:
  virtual ~RequestHandlerArgs() = default;
};

class RequestHandler {
  /*
      This is an abstract class that defines the interface for handling
      requests. It contains a pure virtual function handle_request() that must
      be implemented by any derived class. The function is responsible for
      processing the request and generating a response.
  */
 public:
  enum class HandlerType {
    ECHO_REQUEST_HANDLER,
    STATIC_REQUEST_HANDLER,
    NOT_FOUND_REQUEST_HANDLER,
    CRUD_REQUEST_HANDLER,
    HEALTH_REQUEST_HANDLER,
  };  // Enum to represent the type of handler

  static std::string handler_type_to_string(HandlerType type) {
    switch (type) {
      case HandlerType::ECHO_REQUEST_HANDLER:
        return "EchoHandler";
      case HandlerType::STATIC_REQUEST_HANDLER:
        return "StaticHandler";
      case HandlerType::NOT_FOUND_REQUEST_HANDLER:
        return "NotFoundHandler";
      case HandlerType::CRUD_REQUEST_HANDLER:
        return "CrudHandler";
      case HandlerType::HEALTH_REQUEST_HANDLER:
        return "HealthHandler";
      default:
        return "UnknownHandler";
    }
  }

  virtual ~RequestHandler() = default;
  virtual std::unique_ptr<Response> handle_request(const Request &req) = 0;
  virtual HandlerType get_type() const = 0;
};

#endif