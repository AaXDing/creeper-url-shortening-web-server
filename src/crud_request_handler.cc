// A request handler that supports Create, Read, Update, Delete, and List
// operations for JSON entities.

#include "crud_request_handler.h"

#include "config_parser.h"
#include "logging.h"
#include "registry.h"
#include <boost/filesystem.hpp>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

REGISTER_HANDLER("CrudHandler", CrudRequestHandler);

CrudRequestHandler::CrudRequestHandler(const std::string &base_uri,
                                       const std::string &data_path) {
  base_uri_ = base_uri;
  data_path_path_ = data_path;
  std::filesystem::create_directories(data_path_path_);
}

std::string CrudRequestHandler::extract_entity(const std::string &uri) const {
  std::regex re(base_uri_ + "/([^/]+)");
  std::smatch match;
  return (std::regex_search(uri, match, re) && match.size() > 1)
             ? match[1].str()
             : "";
}

std::string CrudRequestHandler::extract_id(const std::string &uri) const {
  std::regex re(base_uri_ + "/[^/]+/([^/]+)");
  std::smatch match;
  return (std::regex_search(uri, match, re) && match.size() > 1)
             ? match[1].str()
             : "";
}

// Generate next available integer ID by checking existing filenames
int CrudRequestHandler::get_next_available_id(
    const std::string &entity_dir) const {
  int max_id = 0;
  for (const auto &entry : std::filesystem::directory_iterator(entity_dir)) {
    try {
      int id = std::stoi(entry.path().filename().string());
      if (id > max_id)
        max_id = id;
    } catch (...) {
      continue; // skip files that can't be parsed as integers
    }
  }

  if (max_id >= INT_MAX) {
    LOG(error) << "Exceeded maximum ID value: cannot assign new ID";
    return -1; // or throw, or handle differently if needed
  }

  return max_id + 1;
}

// Return JSON list of IDs (filenames) in a directory
std::string CrudRequestHandler::list_ids(const std::string &entity_dir) const {
  std::ostringstream oss;
  oss << "[";
  bool first = true;
  for (const auto &entry : std::filesystem::directory_iterator(entity_dir)) {
    if (!first)
      oss << ", ";
    oss << "\"" << entry.path().filename().string() << "\"";
    first = false;
  }
  oss << "]";
  return oss.str();
}

std::unique_ptr<Response>
CrudRequestHandler::handle_request(const Request &req) {
  auto res = std::make_unique<Response>();

  const std::string method = req.method;
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_path_ + "/" + entity;

  if (entity.empty()) {
    LOG(warning) << "Malformed CRUD request: no entity in URI.";
    *res = STOCK_RESPONSE.at(400);
    res->body = "Invalid URI: missing entity";
    return res;
  }

  std::filesystem::create_directories(entity_dir);

  if (method == "POST" && id.empty()) {
    int new_id = get_next_available_id(entity_dir);
    std::string filepath = entity_dir + "/" + std::to_string(new_id);
    std::ofstream ofs(filepath);
    if (!ofs) {
      LOG(error) << "Failed to create file: " << filepath;
      *res = STOCK_RESPONSE.at(500);
      return res;
    }
    ofs << req.body;
    ofs.close();

    res->status_code = 200;
    res->content_type = "application/json";
    res->body = "{\"id\": " + std::to_string(new_id) + "}";
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_message = "OK";
    LOG(info) << "Created new " << entity << " with ID " << new_id;
    return res;
  }

  if (method == "GET" && !id.empty()) {
    std::string filepath = entity_dir + "/" + id;
    std::ifstream ifs(filepath);
    if (!ifs) {
      LOG(warning) << "GET failed: file not found at " << filepath;
      *res = STOCK_RESPONSE.at(404);
      res->body = "ID not found";
      return res;
    }

    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    res->status_code = 200;
    res->content_type = "application/json";
    res->body = buffer.str();
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_message = "OK";
    return res;
  }

  if (method == "GET" && id.empty()) {
    res->status_code = 200;
    res->content_type = "application/json";
    res->body = list_ids(entity_dir);
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_message = "OK";
    return res;
  }

  LOG(warning) << "Unsupported CRUD operation: " << method << " " << uri;
  *res = STOCK_RESPONSE.at(400);
  res->body = "Unsupported operation or malformed request";
  return res;
}

bool CrudRequestHandler::check_location(
    std::shared_ptr<NginxConfigStatement> statement, NginxLocation &location) {
  if (!(statement->child_block_ &&
        statement->child_block_->statements_.size() == 1)) {
    LOG(error) << "CrudHandler must have exactly one child statement: "
                  "data_path <path>;";
    return false;
  }
  auto data_path_stmt = statement->child_block_->statements_[0];
  if (!(data_path_stmt->tokens_.size() == 2 &&
        data_path_stmt->tokens_[0] == "data_path")) {
    LOG(error) << "CrudHandler must have child statement: data_path <path>;";
    return false;
  }
  location.root = data_path_stmt->tokens_[1];
  // Error if data_path path has trailing slash (except for "/")
  if (location.root.value().back() == '/' &&
      location.root.value() != "/") {
    LOG(error) << "CrudHandler data_path path cannot have trailing slash";
    return false;
  }

  // Normalize to absolute path
  boost::filesystem::path abs_path =
      boost::filesystem::absolute(location.root.value());
  location.root = abs_path.string();

  // If path exists, ensure it's a directory
  if (boost::filesystem::exists(abs_path)) {
    if (!boost::filesystem::is_directory(abs_path)) {
      LOG(error) << "CrudHandler data_path exists but is not a directory: "
                 << abs_path;
      return false;
    }
  }
  return true;
}

RequestHandler::HandlerType CrudRequestHandler::get_type() const {
  return RequestHandler::HandlerType::CRUD_REQUEST_HANDLER;
}
