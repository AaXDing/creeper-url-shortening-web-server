#ifndef ECHO_RESPONSE_H
#define ECHO_RESPONSE_H

#include <string>

// Constructs an HTTP response that echoes the provided request_msg.
// If valid is true, returns a 200 OK response echoing the original request;
// otherwise, returns a 400 Bad Request response.
// Modified based on LLM models
std::string make_echo_response(const std::string &http_version,
                               const std::string &request_msg,
                               bool valid);

#endif  // ECHO_RESPONSE_H
