#ifndef REQUEST_HANDLER_DISPATCHER_H
#define REQUEST_HANDLER_DISPATCHER_H

#include <string>
#include <tuple>
#include <unordered_map>

#include "config_parser.h"
#include "echo_request_handler.h"
#include "http_header.h"
#include "registry.h"
#include "request_handler.h"

using RequestHandlerFactoryPtr =
   std::shared_ptr<RequestHandlerFactory>;
                      
using RequestHandlerFactoryAndWorkersPtr =
    std::shared_ptr<std::tuple<RequestHandlerFactoryPtr,
                               std::string, std::string>>;

class RequestHandlerDispatcher {
 public:
  RequestHandlerDispatcher(const NginxConfig& config);
  ~RequestHandlerDispatcher();

  std::unique_ptr<RequestHandler> get_handler(const Request& req);

 private:
  bool add_routes(const NginxConfig& config);
  bool add_route(const NginxLocation& location);

  std::string longest_prefix_match(const std::string& url);


  std::unordered_map<std::string, RequestHandlerFactoryAndWorkersPtr>
      routes_;


  friend class RequestHandlerDispatcherTest;
};

#endif  // REQUEST_HANDLER_DISPATCHER_H