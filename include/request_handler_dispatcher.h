#ifndef HANDLER_DISPATCHER_H
#define HANDLER_DISPATCHER_H

#include <memory>
#include <unordered_map>
#include <string>

#include "request_handler.h"
#include "echo_request_handler.h"
#include "request_handler.h"
#include "http_header.h"
#include "config_parser.h"

class RequestHandlerDispatcher {
 public:
  RequestHandlerDispatcher(const NginxConfig& config);
  ~RequestHandlerDispatcher();

  std::shared_ptr<RequestHandler> get_handler(const Request& req) const;
  size_t get_num_handlers();

 private:
  void add_handlers(const NginxConfig& config); 
  bool add_handler(std::string uri, std::unique_ptr<NginxConfig>& config);
  std::unordered_map<std::string, std::shared_ptr<RequestHandler>> handlers_; // map that maps uri to handler

  friend class RequestHandlerDispatcherTest;
};

#endif