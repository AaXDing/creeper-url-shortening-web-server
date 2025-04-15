#include "echo_response.h"   
#include <sstream>           
#include <string>

// Constructs an HTTP response based on the provided request message.
// Code below modified based on LLM outputs
// Parameters:
//   http_version: The HTTP version to use in the response (e.g., "HTTP/1.1").
//   request_msg: The original HTTP request message, used as the body if valid.
//   valid: A boolean indicating whether the request was considered valid.
// Returns:
//   A string containing the full HTTP response.
std::string make_echo_response(const std::string &http_version,
                               const std::string &request_msg,
                               bool valid) {
    // Create an output string stream for assembling the HTTP response.
    std::ostringstream oss;
    int status_code;
    std::string status_message;
    std::string body;
    
    if (valid) {
        // If the request is valid, prepare a 200 OK response.
        status_code = 200;
        status_message = "OK";
        body = request_msg;
    } else {
        // If the request is invalid, prepare a 400 Bad Request response.
        status_code = 400;
        status_message = "Bad Request";
        body = "400 Bad Request";       // Define the body for invalid requests.
    }
    std::string content_type = "text/plain"; // Content type is text/plain
    // Final response
    oss << http_version << " " << status_code << " " << status_message << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n" << body;
    return oss.str();
}
