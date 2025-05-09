#include "static_request_handler.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <memory>

#include "logging.h"
#include "request_handler.h"
#include "registry.h"

REGISTER_HANDLER("StaticHandler", StaticRequestHandler);

StaticRequestHandler* StaticRequestHandler::create(std::string base_uri,
                                                   std::string root_path) {
  // Assume root path is valid

  // // path must start with a slash or be a single slash
  // bool valid = root_path.size() >= 1 && root_path[0] == '/' &&
  //              (root_path == "/" || root_path.back() != '/');
  // LOG(debug) << "Initial root_path valid=" << valid;

  // if (valid && root_path != "/") {
  //   for (size_t i = 1; i < root_path.size(); ++i) {
  //     if (root_path[i] == '/' && root_path[i - 1] == '/') {
  //       valid = false;
  //       break;
  //     }
  //   }
  //   LOG(debug) << "Post-sanitization valid=" << valid;
  // }

  // if (!valid) {
  //   LOG(error) << "Invalid root path \"" << root_path << "\" for URI \""
  //              << base_uri << "\"";
  //   LOG(error)
  //       << "Root path must start with a slash, not end with one (unless it's
  //       a "
  //          "single slash), and must not contain consecutive slashes.";
  //   return nullptr;
  // }
  return new StaticRequestHandler(std::move(base_uri), std::move(root_path));
}

StaticRequestHandler::StaticRequestHandler(std::string base_uri,
                                           std::string root_path)
    : base_uri_(std::move(base_uri)), root_path_(std::move(root_path)) {}

Response StaticRequestHandler::handle_request(Request& req) const {
  // remove the first / field
  // and add the root path
  // to the file path

  // e.g. if root /var/www}
  // e.g. /static/test1/test.txt -> ../data/var/www/test1/test.txt
  std::string file_path = generate_file_path(req.uri);
  LOG(debug) << "Computed file_path='" << file_path << "'";

  Response res;

  // Check if the file exists and is a regular file
  if (!boost::filesystem::exists(file_path) ||
      !boost::filesystem::is_regular_file(file_path)) {
    LOG(warning) << "File not found or not a regular file: " << file_path;
    res = STOCK_RESPONSE.at(404);
  } else {
    LOG(info) << "Serving static file: " << file_path;

    // Open the file in binary mode
    std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
      LOG(error) << "Failed to open file: " << file_path;
      res = STOCK_RESPONSE.at(404);
    }

    res.version = req.valid ? req.version : HTTP_VERSION;
    res.status_code = 200;
    res.status_message = "OK";
    res.content_type = get_file_content_type(file_path);
    // Read the file content into the response body
    std::string body((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    res.body = body;
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

RequestHandler::HandlerType StaticRequestHandler::get_type() const {
  return RequestHandler::HandlerType::STATIC_REQUEST_HANDLER;
}