#include "static_request_handler.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <memory>

#include "logging.h"
#include "registry.h"
#include "request_handler.h"

REGISTER_HANDLER("StaticHandler", StaticRequestHandler);

StaticRequestHandler::StaticRequestHandler(std::string base_uri,
                                           std::string root_path)
    : base_uri_(std::move(base_uri)), root_path_(std::move(root_path)) {}

std::unique_ptr<Response> StaticRequestHandler::handle_request(
    const Request& req) {
  // remove the first / field
  // and add the root path
  // to the file path

  // e.g. if root /var/www}
  // e.g. /static/test1/test.txt -> ../data/var/www/test1/test.txt
  std::string file_path = generate_file_path(req.uri);
  LOG(debug) << "Computed file_path='" << file_path << "'";

  auto res = std::make_unique<Response>();

  // Check if the file exists and is a regular file
  if (!boost::filesystem::exists(file_path) ||
      !boost::filesystem::is_regular_file(file_path)) {
    LOG(warning) << "File not found or not a regular file: " << file_path;
    *res = STOCK_RESPONSE.at(404);
  } else {
    LOG(info) << "Serving static file: " << file_path;

    // Open the file in binary mode
    std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
      LOG(error) << "Failed to open file: " << file_path;
      *res = STOCK_RESPONSE.at(404);
    }

    res->version = req.valid ? req.version : HTTP_VERSION;
    res->status_code = 200;
    res->status_message = "OK";
    res->content_type = get_file_content_type(file_path);
    // Read the file content into the response body
    std::string body((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    res->body = std::move(body);
  }

  return res;
}

std::string StaticRequestHandler::generate_file_path(
    const std::string& uri) const {
  std::string file_path = "";
  std::string root_path = root_path_;

  // Remove base URI from the request URI
  // e.g. Base URI /static
  // /static/test1/test.txt -> /test1/test.txt
  file_path = uri.substr(base_uri_.size());
  LOG(debug) << "Path after base_uri strip='" << file_path << "'";

  // Remove trailing slashes
  while (file_path.back() == '/') {
    file_path.pop_back();
  }

  // Add root path in front of the file path
  // e.g. root_path_ ./var/www
  // e.g. /test1/test.txt -> ./var/www/test1/test.txt
  file_path = root_path + file_path;
  LOG(debug) << "Assembled path='" << file_path << "'";

  return file_path;
}

std::string StaticRequestHandler::get_file_content_type(
    const std::string& file_path) const {
  std::string file_extension = "";

  // File path without the leading "."
  std::string pure_file_path = file_path.substr(root_path_.size());

  // Extract the file extension from the file path
  size_t pos = pure_file_path.find_last_of('.');
  if (pos != std::string::npos) {
    file_extension = file_path.substr(pos + 1 + root_path_.size());
    LOG(debug) << "file_extension='" << file_extension << "'";
  }

  // Set the content type based on the file extension
  auto it = CONTENT_TYPE.find(file_extension);
  if (it != CONTENT_TYPE.end()) {
    LOG(debug) << "content_type='" << it->second << "'";
    return it->second;
  }

  LOG(warning) << "Unknown extension '" << file_extension
               << "'; defaulting to application/octet-stream";
  return "application/octet-stream";  // Default content type
}

bool StaticRequestHandler::check_location(
    std::shared_ptr<NginxConfigStatement> statement, NginxLocation& location) {
  if (statement->child_block_->statements_.size() == 1 &&
      statement->child_block_->statements_[0]->tokens_.size() == 2 &&
      statement->child_block_->statements_[0]->tokens_[0] == "root") {
    location.root = statement->child_block_->statements_[0]->tokens_[1];
    
    // error if root path has trailing slash
    if (location.root.value().back() == '/' && location.root.value() != "/") {
      LOG(error) << "Root path cannot have trailing slash";
      return false;
    }

    // normalize path if it is a relative path
    if (boost::filesystem::path(location.root.value()).is_relative()) {
      try {
        location.root.value() =
            boost::filesystem::canonical(location.root.value()).string();
      } catch (const boost::filesystem::filesystem_error& e) {
        LOG(error) << "Root path does not exist: " << location.root.value();
        return false;
      }
    } else {
      try {
        // Check if absolute path exists and is accessible
        if (!boost::filesystem::exists(location.root.value())) {
          LOG(error) << "Root path does not exist: " << location.root.value();
          return false;
        }
        // Convert to canonical form to resolve any symlinks and normalize
        // the path
        location.root.value() =
            boost::filesystem::canonical(location.root.value()).string();
      } catch (const boost::filesystem::filesystem_error& e) {
        LOG(error) << "Error accessing root path: " << location.root.value()
                   << " - " << e.what();
        return false;
      }
    }

  } else {
    LOG(error) << "StaticHandler must have exactly one argument with "
                  "root directive";
    return false;
  }
  return true;
}

RequestHandler::HandlerType StaticRequestHandler::get_type() const {
  return RequestHandler::HandlerType::STATIC_REQUEST_HANDLER;
}