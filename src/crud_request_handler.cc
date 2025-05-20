// A request handler that supports Create, Read, Update, Delete, and List
// operations for JSON entities.

#include "crud_request_handler.h"

#include "config_parser.h"
#include "logging.h"
#include "registry.h"
#include <boost/filesystem.hpp>

#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

REGISTER_HANDLER("CrudHandler", CrudRequestHandler);

CrudRequestHandler::CrudRequestHandler(const std::string &base_uri,
                                       const std::string &data_path)
{
  base_uri_ = base_uri;
  data_path_path_ = data_path;
  std::filesystem::create_directories(data_path_path_);
}

enum HTTP_Method CrudRequestHandler::get_method(const std::string &method) const
{
  if (method == "POST")
  {
    return POST;
  }
  else if (method == "GET")
  {
    return GET;
  }
  else if (method == "PUT")
  {
    return PUT;
  }
  else if (method == "DELETE")
  {
    return DELETE;
  }
  else
  {
    return INVALID_METHOD;
  }
}

std::string CrudRequestHandler::extract_entity(const std::string &uri) const
{
  std::regex re(base_uri_ + "/([^/]+)");
  std::smatch match;
  return (std::regex_search(uri, match, re) && match.size() > 1)
             ? match[1].str()
             : "";
}

std::string CrudRequestHandler::extract_id(const std::string &uri) const
{
  std::regex re(base_uri_ + "/[^/]+/([^/]+)");
  std::smatch match;
  return (std::regex_search(uri, match, re) && match.size() > 1)
             ? match[1].str()
             : "";
}

// Generate next available integer ID by checking existing filenames
int CrudRequestHandler::get_next_available_id(
    const std::string &entity_dir) const
{
  int max_id = 0;
  for (const auto &entry : std::filesystem::directory_iterator(entity_dir))
  {
    try
    {
      int id = std::stoi(entry.path().filename().string());
      if (id > max_id)
        max_id = id;
    }
    catch (...)
    {
      continue; // skip files that can't be parsed as integers
    }
  }

  if (max_id >= INT_MAX)
  {
    LOG(error) << "Exceeded maximum ID value: cannot assign new ID";
    return -1; // or throw, or handle differently if needed
  }

  return max_id + 1;
}

// Return JSON list of IDs (filenames) in a directory
std::string CrudRequestHandler::list_ids(const std::string &entity_dir) const
{
  std::ostringstream oss;
  oss << "[";
  bool first = true;
  for (const auto &entry : std::filesystem::directory_iterator(entity_dir))
  {
    if (!first)
      oss << ", ";
    oss << "\"" << entry.path().filename().string() << "\"";
    first = false;
  }
  oss << "]";
  return oss.str();
}

std::string to_lower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool is_json_content_type(const Request &req)
{
  for (const auto &header : req.headers)
  {
    // HTTP Headers are case-insensitive
    if (to_lower(header.name) == to_lower("Content-Type"))
    {
      return header.value == "application/json";
    }
  }
  return false; // Missing header = invalid
}

bool is_valid_json(const std::string &body)
{
  boost::json::error_code ec;
  boost::json::parse(body, ec);
  return !ec;
}

std::unique_ptr<Response> CrudRequestHandler::handle_post(const Request &req)
{
  // Validate content type and JSON structure
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string entity_dir = data_path_path_ + "/" + entity;

  if (!is_json_content_type(req))
  {
    *res = STOCK_RESPONSE.at(415); // Unsupported Media Type
    res->body = "Content-Type must be application/json";
    return res;
  }

  boost::json::error_code ec;
  boost::json::value parsed = boost::json::parse(req.body, ec);
  if (ec)
  {
    LOG(warning) << "Invalid JSON in POST: " << ec.message();
    *res = STOCK_RESPONSE.at(400);
    res->body = "Invalid JSON body";
    return res;
  }

  std::filesystem::create_directories(entity_dir);
  int new_id = get_next_available_id(entity_dir);
  std::string filepath = entity_dir + "/" + std::to_string(new_id);

  std::ofstream ofs(filepath);
  if (!ofs)
  {
    LOG(error) << "Failed to create file: " << filepath;
    *res = STOCK_RESPONSE.at(500);
    res->body = "Failed to create storage file";
    return res;
  }

  // Write normalized JSON to file
  ofs << boost::json::serialize(parsed);
  ofs.close();

  res->status_code = 201;
  res->status_message = "Created";
  res->version = req.valid ? req.version : HTTP_VERSION;
  res->content_type = "application/json";
  res->body = "{\"id\": " + std::to_string(new_id) + "}";
  LOG(info) << "Created new " << entity << " with ID " << new_id;
  return res;
}

std::unique_ptr<Response> CrudRequestHandler::handle_get(const Request &req)
{
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_path_ + "/" + entity;
  // get an entity
  if (!id.empty())
  {
    std::string filepath = entity_dir + "/" + id;
    std::ifstream ifs(filepath);
    if (!ifs)
    {
      LOG(warning) << "GET failed: file not found at " << filepath;
      *res = STOCK_RESPONSE.at(404);
      res->body = "ID not found";
      return res;
    }

    std::ostringstream raw_stream;
    raw_stream << ifs.rdbuf();
    std::string raw_json = raw_stream.str();

    // Attempt to parse and re-serialize using Boost.JSON
    boost::json::error_code ec;
    boost::json::value parsed = boost::json::parse(raw_json, ec);
    if (ec)
    {
      LOG(warning) << "Failed to parse JSON from file: " << filepath << " — " << ec.message();
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
  else
  {
    if (!std::filesystem::exists(entity_dir))
    {
      *res = STOCK_RESPONSE.at(404);
      res->body = "Entity type not found";
      return res;
    }
    res->status_code = 200;
    res->content_type = "application/json";
    res->body = list_ids(entity_dir);
    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_message = "OK";
    return res;
  }
}

std::unique_ptr<Response> CrudRequestHandler::handle_put(const Request &req)
{
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_path_ + "/" + entity;
  if (!id.empty())
  {
    // Validate content type and JSON structure
    if (!is_json_content_type(req))
    {
      *res = STOCK_RESPONSE.at(415); // Unsupported Media Type
      res->body = "Content-Type must be application/json";
      return res;
    }

    boost::json::error_code ec;
    boost::json::value parsed = boost::json::parse(req.body, ec);
    if (ec)
    {
      LOG(warning) << "Invalid JSON in PUT: " << ec.message();
      *res = STOCK_RESPONSE.at(400);
      res->body = "Invalid JSON body";
      return res;
    }

    std::filesystem::create_directories(entity_dir);
    std::string filepath = entity_dir + "/" + id;
    bool file_previously_existed = std::filesystem::exists(filepath);
    if (file_previously_existed)
    {
      LOG(info) << "File already exists at: " << filepath;
      LOG(info) << "Using full replace update mode";
      std::ofstream ofs(filepath, std::ios::trunc);
      if (!ofs)
      {
        LOG(error) << "Failed to open/overwrite: " << filepath;
        *res = STOCK_RESPONSE.at(500);
        res->body = "Failed to update storage file";
        return res;
      }
      // Write normalized JSON to file
      ofs << boost::json::serialize(parsed);
      ofs.close();
    }
    else
    {
      LOG(info) << "Creating new file at: " << filepath;
      std::ofstream ofs(filepath);
      if (!ofs)
      {
        LOG(error) << "Failed to create file: " << filepath;
        *res = STOCK_RESPONSE.at(500);
        res->body = "Failed to create storage file";
        return res;
      }

      // Write normalized JSON to file
      ofs << boost::json::serialize(parsed);
      ofs.close();
    }
    if(file_previously_existed) {
      res->status_code = 200;
      res->status_message = "OK";
      res->version = req.valid ? req.version : HTTP_VERSION;
      LOG(info) << "Updated " << entity << " with ID " << id;
    }
    else {
      res->status_code = 201;
      res->status_message = "Created";
      res->version = req.valid ? req.version : HTTP_VERSION;
      LOG(info) << "Created " << entity << " with ID " << id;
    }
    return res;
  }
  // there's no ID
  else
  {
    LOG(warning) << "CRUD PUT failed: ID not specified";
    // Follows CrudCrud website update with no ID
    res->status_code = 405;
    res->body = "ID must be specified for PUT";
    return res;
  }
}

std::unique_ptr<Response> CrudRequestHandler::handle_delete(const Request &req)
{
  auto res = std::make_unique<Response>();
  const std::string uri = req.uri;
  const std::string entity = extract_entity(uri);
  const std::string id = extract_id(uri);
  const std::string entity_dir = data_path_path_ + "/" + entity;

  if (!id.empty())
  {
    std::string filepath = entity_dir + "/" + id;
    if (std::filesystem::remove(filepath))
    {
      res->status_code = 204;
      res->status_message = "No Content";
      res->version = req.valid ? req.version : HTTP_VERSION;
      LOG(info) << "Deleted " << entity << " with ID " << id;
    }
    else
    {
      LOG(warning) << "DELETE failed: file not found at " << filepath;
      *res = STOCK_RESPONSE.at(404);
      res->body = "ID not found";
    }
  }
  else
  {
    LOG(warning) << "CRUD DELETE failed: ID not specified";
    res->status_code = 405;
    res->body = "ID must be specified for DELETE";
  }
  return res;
}

std::unique_ptr<Response>
CrudRequestHandler::handle_request(const Request &req)
{
  auto res = std::make_unique<Response>();
  const std::string entity = extract_entity(req.uri);

  if (entity.empty())
  {
    LOG(warning) << "Malformed CRUD request: no entity in URI.";
    *res = STOCK_RESPONSE.at(400);
    res->body = "Invalid URI: missing entity";
    return res;
  }

  switch (get_method(req.method))
  {
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
    LOG(warning) << "Unsupported CRUD operation: " << req.method << " " << req.uri;
    *res = STOCK_RESPONSE.at(400);
    res->body = "Unsupported operation or malformed request";
    return res;
  }
}

bool CrudRequestHandler::check_location(
    std::shared_ptr<NginxConfigStatement> statement, NginxLocation &location)
{
  if (!(statement->child_block_ &&
        statement->child_block_->statements_.size() == 1))
  {
    LOG(error) << "CrudHandler must have exactly one child statement: "
                  "data_path <path>;";
    return false;
  }
  auto data_path_stmt = statement->child_block_->statements_[0];
  if (!(data_path_stmt->tokens_.size() == 2 &&
        data_path_stmt->tokens_[0] == "data_path"))
  {
    LOG(error) << "CrudHandler must have child statement: data_path <path>;";
    return false;
  }
  location.root = data_path_stmt->tokens_[1];
  // Error if data_path path has trailing slash (except for "/")
  if (location.root.value().back() == '/' && location.root.value() != "/")
  {
    LOG(error) << "CrudHandler data_path path cannot have trailing slash";
    return false;
  }

  // Normalize to absolute path
  boost::filesystem::path abs_path =
      boost::filesystem::absolute(location.root.value());
  location.root = abs_path.string();

  // If path exists, ensure it's a directory
  if (boost::filesystem::exists(abs_path))
  {
    if (!boost::filesystem::is_directory(abs_path))
    {
      LOG(error) << "CrudHandler data_path exists but is not a directory: "
                 << abs_path;
      return false;
    }
  }
  return true;
}

RequestHandler::HandlerType CrudRequestHandler::get_type() const
{
  return RequestHandler::HandlerType::CRUD_REQUEST_HANDLER;
}
