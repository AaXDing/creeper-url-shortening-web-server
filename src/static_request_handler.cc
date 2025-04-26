#include "static_request_handler.h"

#include <fstream>

std::string StaticRequestHandler::handle_request(Request& req,
                                                 Response& res) const {
  std::string file_path = ".." + req.uri;
  // Remove trailing slashes from the URI
  while (file_path.size() > 0 && file_path[file_path.size() - 1] == '/') {
    file_path.pop_back();  // Remove trailing slashes
  }

  // Check if the file exists
  std::ifstream file(file_path);
  std::string response_str;

  if (file) {
    res.status_code = 200;
    res.status_message = "OK";
    res.version = req.valid ? req.version : HTTP_VERSION;
    res.content_type = "text/plain";
    // Read the file content into the response body
    std::string body((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    res.body = body;

    // Convert the Response object to a string
    response_str = RequestHandler::response_to_string(res);
  } else {
    response_str = STOCK_RESPONSE.at(404);  // Return a 404 Not Found response
  }

  return response_str;
}