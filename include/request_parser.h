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

class request_parser
{
public:
    // Parse raw HTTP request to versino, method, uri, headers
    void parse(request &req, const std::string &raw_request);
};

#endif
