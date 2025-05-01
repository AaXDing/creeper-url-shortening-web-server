#ifndef REQUEST_HANDLER_DISPATCHER_H
#define REQUEST_HANDLER_DISPATCHER_H

#include <string>
#include <unordered_map>

#include "config_parser.h"
#include "echo_request_handler.h"
#include "http_header.h"
#include "request_handler.h"

class RequestHandlerDispatcher {
 public:
  RequestHandlerDispatcher(const NginxConfig& config);
  ~RequestHandlerDispatcher();

  std::shared_ptr<RequestHandler> get_handler(const Request& req) const;
  size_t get_num_handlers();

 private:
  void add_handlers(const NginxConfig& config);
  bool add_handler(std::string uri, std::unique_ptr<NginxConfig>& config);
  std::unordered_map<std::string, std::shared_ptr<RequestHandler>>
      handlers_;  // map that maps uri to handler

  friend class RequestHandlerDispatcherTest;
};

#endif  // REQUEST_HANDLER_DISPATCHER_Hs