#ifndef CRUD_REQUEST_HANDLER_H
#define CRUD_REQUEST_HANDLER_H

#include <memory>

#include "config_parser.h"
#include "http_header.h"
#include "request_handler.h"

class CrudRequestHandler : public RequestHandler {
public:
  CrudRequestHandler(const std::string &base_uri, const std::string &data_path);
  std::unique_ptr<Response> handle_request(const Request &req) override;
  static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                             NginxLocation &location);
  RequestHandler::HandlerType get_type() const override;

  std::string extract_entity(const std::string &uri) const;
  std::string extract_id(const std::string &uri) const;
  int get_next_available_id(const std::string &entity_dir) const;
  std::string list_ids(const std::string &entity_dir) const;

private:
  std::string data_path_path_;
  std::string base_uri_;
};

#endif
