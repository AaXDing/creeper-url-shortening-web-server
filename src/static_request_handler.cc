#include "static_request_handler.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <memory>

#include "logging.h"

StaticRequestHandler::StaticRequestHandler(std::string base_uri,
                                           std::string root_path)
    : base_uri_(std::move(base_uri)), root_path_(std::move(root_path)) {
  // Ensure the root path is valid
}

std::string StaticRequestHandler::handle_request(Request& req,
                                                 Response& res) const {
  // remove the first / field
  // and add the root path
  // to the file path

  // e.g. if root /var/www}
  // e.g. /static/test1/test.txt -> ../data/var/www/test1/test.txt
  std::string file_path = generate_file_path(req.uri);

  std::string response_str;

  // Check if the file exists and is a regular file
  if (!boost::filesystem::exists(file_path) ||
      !boost::filesystem::is_regular_file(file_path)) {
    LOG(warning) << "File not found or not a regular file: " << file_path;
    response_str = STOCK_RESPONSE.at(404);
  } else {
    LOG(info) << "Serving static file: " << file_path;

    // Open the file in binary mode
    std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
      LOG(error) << "Failed to open file: " << file_path;
      return STOCK_RESPONSE.at(500);
    }

    res.status_code = 200;
    res.status_message = "OK";
    res.version = req.valid ? req.version : HTTP_VERSION;
    res.content_type = get_file_content_type(file_path);
    // Read the file content into the response body
    std::string body((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    res.body = body;

    // Convert the Response object to a string
    response_str = RequestHandler::response_to_string(res);
  }

  return response_str;
}

std::string StaticRequestHandler::generate_file_path(
    const std::string& uri) const {
  std::string file_path = "";

  // Remove base URI from the request URI
  file_path = uri.substr(base_uri_.size());
  // Remove leading slashes from the URI
  if (file_path.size() > 1) {
    int pos = file_path.find_first_of('/');
    if (pos != std::string::npos) {
      file_path = file_path.substr(pos + 1);
    }
  }

  // // Add root path in front of the file path
  file_path = "../data" + root_path_ + file_path;

  // Remove trailing slashes from the URI
  while (file_path.size() > 0 && file_path[file_path.size() - 1] == '/') {
    file_path.pop_back();  // Remove trailing slashes
  }

  return file_path;
}

std::string StaticRequestHandler::get_file_content_type(
    const std::string& file_path) const {
  std::string file_extension = "";

  // File path without the leading "../"
  std::string pure_file_path = file_path.substr(3);

  // Extract the file extension from the file path
  size_t pos = pure_file_path.find_last_of('.');
  if (pos != std::string::npos) {
    file_extension = file_path.substr(pos + 1 + 3);  // +3 to skip "../"
  }

  // Set the content type based on the file extension
  auto it = CONTENT_TYPE.find(file_extension);
  if (it != CONTENT_TYPE.end()) {
    return it->second;
  }

  LOG(warning) << "Unknown extension '" << file_extension
               << "'; defaulting to application/octet-stream";
  return "application/octet-stream";  // Default content type
}