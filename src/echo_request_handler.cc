// A class that handles HTTP requests

#include "echo_request_handler.h"

#include "logging.h"

Response EchoRequestHandler::handle_request(Request& req) const {
  Response res;  // Create a new Response object

  if (req.valid) {  // If the request is valid, echo the request
    LOG(info) << "Valid echo request: " << req.method << " " << req.uri;
    res.status_code = 200;
    res.status_message = "OK";
    res.version = req.valid ? req.version : HTTP_VERSION;
    res.content_type = "text/plain";
    res.body = req.to_string();  // Convert the request to a string
  } else {  // If the request is invalid, return a 400 Bad Request response
    LOG(warning) << "Invalid echo request → returning 400 Bad Request";
    res = STOCK_RESPONSE.at(400);  // Return a 400 Bad Request response
  }

  return res;  // Return the response
}

RequestHandler::HandlerType EchoRequestHandler::get_type() const {
  return RequestHandler::HandlerType::ECHO_REQUEST_HANDLER;  // Return the handler type
}