// A class that handles HTTP requests

#include "echo_request_handler.h"

std::string EchoRequestHandler::handle_request(Request& req,
                                               Response& res) const {
  if (req.valid) {  // If the request is valid, echo the request
    res.status_code = 200;
    res.status_message = "OK";
    res.version = req.valid ? req.version : HTTP_VERSION;
    ;
    res.content_type = "text/plain";
    res.body = request_to_string(req);

    return RequestHandler::response_to_string(
        res);  // Convert the Response object to a string
  } else {     // If the request is invalid, return a 400 Bad Request response
    return STOCK_RESPONSE.at(400);  // Return a 400 Bad Request response
  }
}

std::string EchoRequestHandler::request_to_string(const Request& req) const {
  // Construct the HTTP request string from the Request object
  std::string request_str =
      req.method + " " + req.uri + " " + req.version + "\r\n";
  for (const auto& header : req.headers) {
    request_str += header.name + ": " + header.value + "\r\n";
  }
  request_str += "\r\n";
  return request_str;
}