#ifndef CRUD_REQUEST_HANDLER_H
#define CRUD_REQUEST_HANDLER_H

#include <memory>

#include "config_parser.h"
#include "http_header.h"
#include "ientity_storage.h"
#include "request_handler.h"

enum HTTP_Method { POST, GET, PUT, DELETE, INVALID_METHOD };

class CrudRequestHandlerArgs : public RequestHandlerArgs {
 public:
  CrudRequestHandlerArgs(std::string data_path);
  static std::shared_ptr<CrudRequestHandlerArgs> create_from_config(
      std::shared_ptr<NginxConfigStatement> statement);
  std::string get_data_path() const;

 private:
  std::string data_path_;
  std::shared_ptr<IEntityStorage> storage_;
};
class CrudRequestHandler : public RequestHandler {
 public:
  CrudRequestHandler(std::string base_uri,
                     std::shared_ptr<CrudRequestHandlerArgs> args);
  std::unique_ptr<Response> handle_request(const Request &req) override;
  RequestHandler::HandlerType get_type() const override;

  std::string extract_entity(const std::string &uri) const;
  std::string extract_id(const std::string &uri) const;
  int get_next_available_id(const std::string &entity_dir) const;
  std::string list_ids(const std::string &entity_dir) const;
  enum HTTP_Method get_method(const std::string &method) const;
  std::unique_ptr<Response> handle_post(const Request &req);
  std::unique_ptr<Response> handle_get(const Request &req);
  std::unique_ptr<Response> handle_put(const Request &req);
  std::unique_ptr<Response> handle_delete(const Request &req);
  void set_storage(std::shared_ptr<IEntityStorage> storage);

 private:
  std::string data_path_;
  std::string base_uri_;
  std::shared_ptr<IEntityStorage> storage_;
};

#endif
