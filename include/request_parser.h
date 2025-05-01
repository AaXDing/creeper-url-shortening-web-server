// A class that parses HTTP requests
//
// Adopted from Boost Asio example: request_parser.h
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <string>

#include "http_header.h"

class RequestParser {
 public:
  // Parse raw HTTP Request to version, method, uri, headers
  void parse(Request &req, const std::string &raw_request);
};

#endif  // REQUEST_PARSER_H
