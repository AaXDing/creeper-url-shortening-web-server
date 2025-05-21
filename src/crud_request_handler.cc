// A request handler that supports Create, Read, Update, Delete, and List
// operations for JSON entities.

#include "crud_request_handler.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include "config_parser.h"
#include "ientity_storage.h"
#include "logging.h"
#include "real_entity_storage.h"
#include "registry.h"

REGISTER_HANDLER("CrudHandler", CrudRequestHandler);

CrudRequestHandler::CrudRequestHandler(const std::string &base_uri,
                                       const std::string &data_path) {
  base_uri_ = base_uri;
  data_path_ = data_path;
  std::filesystem::create_directories(data_path_);
  storage_ = std::make_shared<RealEntityStorage>(data_path_);
}

void CrudRequestHandler::set_storage(std::shared_ptr<IEntityStorage> storage) {
  storage_ = storage;
}

enum HTTP_Method CrudRequestHandler::get_method(
    const std::string &method) const {
  if (method == "POST") {
    return POST;
  } else if (method == "GET") {
    return GET;
  } else if (method == "PUT") {
    return PUT;
  } else if (method == "DELETE") {
    return DELETE;
  } else {
    return INVALID_METHOD;
  }
}

bool isValidInteger(const std::string &input, int &outValue) {
  try {
    size_t idx;
    outValue = std::stoi(input, &idx);
    // Check if the entire string was used
    return idx == input.size();
  } catch (const std::invalid_argument &) {
    return false;
  } catch (const std::out_of_range &) {
    return false;
  }
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
      if (id > max_id) max_id = id;
    } catch (...) {
      continue;  // skip files that can't be parsed as integers
    }
  }

  if (max_id >= INT_MAX) {
    LOG(error) << "Exceeded maximum ID value: cannot assign new ID";
    return -1;  // or throw, or handle differently if needed
  }

  return max_id + 1;
}

std::string vector_to_json(const std::vector<int> &ids) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < ids.size(); ++i) {
    oss << "\"" << ids[i] << "\"";
    if (i < ids.size() - 1) {
      oss << ",";
    }
  }
  oss << "]";
  return oss.str();
}

// Return JSON list of IDs (filenames) in a directory
std::string CrudRequestHandler::list_ids(const std::string &entity_dir) const {
  std::ostringstream oss;
  oss << "[";
  bool first = true;
  for (const auto &entry : std::filesystem::directory_iterator(entity_dir)) {
    if (!first) oss << ", ";
    oss << "\"" << entry.path().filename().string() << "\"";
    first = false;
  }
  oss << "]";
  return oss.str();
}

std::string to_lower(const std::string &input) {
  std::string result = input;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

bool is_json_content_type(const Request &req) {
  for (const auto &header : req.headers) {
    // HTTP Headers are case-insensitive
    if (to_lower(header.name) == to_lower("Content-Type")) {
      return header.value == "application/json";
    }
  }
  return false;  // Missing header = invalid
}

bool is_valid_json(const std::string &body) {
  boost::json::error_code ec;
  boost::json::parse(body, ec);
  return !ec;
}

std::unique_ptr<Response> CrudRequestHandler::handle_post(const Request &req) {
  // Validate content type and JSON structure
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string entity_dir = data_path_ + "/" + entity;

  if (!is_json_content_type(req)) {
    *res = STOCK_RESPONSE.at(415);  // Unsupported Media Type
    res->body = "Content-Type must be application/json";
    return res;
  }

  boost::json::error_code ec;
  boost::json::value parsed = boost::json::parse(req.body, ec);
  if (ec) {
    LOG(warning) << "Invalid JSON in POST: " << ec.message();
    *res = STOCK_RESPONSE.at(400);
    res->body = "Invalid JSON body";
    return res;
  }

  auto result = storage_->create(entity, req.body);

  res->status_code = 201;
  res->status_message = "Created";
  res->version = req.valid ? req.version : HTTP_VERSION;
  res->content_type = "application/json";
  res->body = "{\"id\": " + std::to_string(result.value()) + "}";
  LOG(info) << "Created new " << entity << " with ID " << result.value();
  return res;
}

std::unique_ptr<Response> CrudRequestHandler::handle_get(const Request &req) {
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_ + "/" + entity;
  // get an entity
  if (!id.empty()) {
    LOG(debug) << "ID Present, searching for entity: " << entity
               << " with ID: " << id;

    std::string filepath = entity_dir + "/" + id;

    int id_int;
    if (!isValidInteger(id, id_int)) {
      LOG(warning) << "GET failed, ID not found: " << id;
      *res = STOCK_RESPONSE.at(404);
      res->body = "ID not found";
      return res;
    }

    auto result = storage_->retrieve(entity, std::stoi(id));
    if (!result.has_value()) {
      LOG(warning) << "GET failed: file not found at " << filepath;
      *res = STOCK_RESPONSE.at(404);
      res->body = "ID not found";
      return res;
    }

    LOG(info) << "GET result: " << result.value();

    // Attempt to parse and re-serialize using Boost.JSON
    boost::json::error_code ec;
    boost::json::value parsed = boost::json::parse(result.value(), ec);
    if (ec) {
      LOG(warning) << "Failed to parse JSON from file: " << filepath << " — "
                   << ec.message();
      *res = STOCK_RESPONSE.at(500);
      res->body = "Stored JSON could not be parsed";
      return res;
    }

    std::string normalized = boost::json::serialize(parsed);

    res->status_code = 200;
    res->content_type = "application/json";
    res->body = normalized;
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_message = "OK";
    return res;
  }
  // there's no id so we're in list mode
  else {
    LOG(debug) << "ID not present, listing entity: " << entity;
    std::vector<int> ids = storage_->list(entity);
    if (ids.empty()) {
      LOG(warning) << "GET failed: Entity type not found for " << entity;
      *res = STOCK_RESPONSE.at(404);
      res->body = "Entity type not found";
      return res;
    }
    res->status_code = 200;
    res->content_type = "application/json";
    // res->body = list_ids(entity_dir);
    res->body = vector_to_json(ids);
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_message = "OK";
    return res;
  }
}

std::unique_ptr<Response> CrudRequestHandler::handle_put(const Request &req) {
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_ + "/" + entity;
  if (!id.empty()) {
    // Validate content type and JSON structure
    if (!is_json_content_type(req)) {
      *res = STOCK_RESPONSE.at(415);  // Unsupported Media Type
      res->body = "Content-Type must be application/json";
      return res;
    }

    boost::json::error_code ec;
    boost::json::value parsed = boost::json::parse(req.body, ec);
    if (ec) {
      LOG(warning) << "Invalid JSON in PUT: " << ec.message();
      *res = STOCK_RESPONSE.at(400);
      res->body = "Invalid JSON body";
      return res;
    }

    bool file_previously_existed =
        storage_->retrieve(entity, std::stoi(id)).has_value();
    bool result = storage_->update(entity, std::stoi(id), req.body);
    if (!result) {
      // result should always be true because the default is to create
      LOG(warning) << "PUT failed with " << entity_dir + "/" + id;
      *res = STOCK_RESPONSE.at(500);
      res->body = "PUT failed";
      return res;
    }
    if (file_previously_existed) {
      res->status_code = 200;
      res->status_message = "OK";
      res->version = req.valid ? req.version : HTTP_VERSION;
      LOG(info) << "Updated " << entity << " with ID " << id;
    } else {
      res->status_code = 201;
      res->status_message = "Created";
      res->version = req.valid ? req.version : HTTP_VERSION;
      LOG(info) << "Created " << entity << " with ID " << id;
    }
    return res;
  }
  // there's no ID
  else {
    LOG(warning) << "CRUD PUT failed: ID not specified";
    // Follows CrudCrud website update with no ID
    res->status_code = 405;
    res->body = "ID must be specified for PUT";
    return res;
  }
}

std::unique_ptr<Response> CrudRequestHandler::handle_delete(
    const Request &req) {
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_ + "/" + entity;

  if (!id.empty()) {
    // Validate ID
    int id_int;
    if (!isValidInteger(id, id_int)) {
      LOG(warning) << "DELETE failed: invalid ID: " << id;
      *res = STOCK_RESPONSE.at(404);
      // res->body = "Invalid ID";
      res->body = "ID not found";
      return res;
    }
    bool result = storage_->remove(entity, std::stoi(id));
    if (!result) {
      LOG(warning) << "DELETE failed: file not found at "
                   << entity_dir + "/" + id;
      *res = STOCK_RESPONSE.at(404);
      res->body = "ID not found";
      return res;
    } else {
      res->status_code = 204;
      res->status_message = "No Content";
      res->version = req.valid ? req.version : HTTP_VERSION;
      LOG(info) << "Deleted " << entity << " with ID " << id;
    }
  } else {
    LOG(warning) << "CRUD DELETE failed: ID not specified";
    res->status_code = 405;
    res->body = "ID must be specified for DELETE";
  }
  return res;
}

std::unique_ptr<Response> CrudRequestHandler::handle_request(
    const Request &req) {
  auto res = std::make_unique<Response>();
  const std::string entity = extract_entity(req.uri);

  if (entity.empty()) {
    LOG(warning) << "Malformed CRUD request: no entity in URI.";
    *res = STOCK_RESPONSE.at(400);
    res->body = "Invalid URI: missing entity";
    return res;
  }

  switch (get_method(req.method)) {
    case POST:
      return handle_post(req);
    case GET:
      return handle_get(req);
    case PUT:
      return handle_put(req);
    case DELETE:
      return handle_delete(req);
    case INVALID_METHOD:
    default:
      LOG(warning) << "Unsupported CRUD operation: " << req.method << " "
                   << req.uri;
      *res = STOCK_RESPONSE.at(400);
      res->body = "Unsupported operation or malformed request";
      return res;
  }
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
  if (location.root.value().back() == '/' && location.root.value() != "/") {
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
