// A class that parses HTTP requests
//
// Adopted from the Boost Asio example: request_parser.cpp
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "http_header.h"
#include "request_parser.h"
#include <string>

request_parser::request_parser()
    : state_(method_start) {}

request_parser::result_type request_parser::consume(request &req, char input)
{
    // finite state machine of the parser, pretty self explanatory
    // state_ is the current state of the parser
    // input is the current character being parsed
    // req is the request object being parsed
    // the parser starts in the method_start state
    switch (state_)
    {
    case method_start:
        if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return invalid;
        }
        else
        {
            state_ = method;
            req.method.push_back(input);
            return indeterminate;
        }
    case method:
        if (input == ' ')
        {
            state_ = uri;
            return indeterminate;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return invalid;
        }
        else
        {
            req.method.push_back(input);
            return indeterminate;
        }
    case uri:
        if (input == ' ')
        {
            state_ = http_version_h;
            return indeterminate;
        }
        else if (is_ctl(input))
        {
            return invalid;
        }
        else
        {
            req.uri.push_back(input);
            return indeterminate;
        }
    case http_version_h:
        if (input == 'H')
        {
            state_ = http_version_t_1;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version_t_1:
        if (input == 'T')
        {
            state_ = http_version_t_2;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version_t_2:
        if (input == 'T')
        {
            state_ = http_version_p;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version_p:
        if (input == 'P')
        {
            state_ = http_version_slash;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version_slash:
        if (input == '/')
        {
            req.version = "HTTP/";
            state_ = http_version_start;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version_start:
        if (is_digit(input))
        {
            req.version.push_back(input);
            state_ = http_version;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version:
        if (input == '.')
        {
            state_ = http_version;
            req.version.push_back(input);
            return indeterminate;
        }
        else if (is_digit(input))
        {
            req.version.push_back(input);
            state_ = http_version_end;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case http_version_end:
        if (input == '\r')
        {
            state_ = expecting_newline_1;
            return indeterminate;
        }
        else
        {
            return invalid;
        }

    case expecting_newline_1:
        if (input == '\n')
        {
            state_ = header_line_start;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case header_line_start:
        if (input == '\r')
        {
            state_ = expecting_newline_3;
            return indeterminate;
        }
        else if (!req.headers.empty() && (input == ' ' || input == '\t'))
        {
            state_ = header_lws;
            return indeterminate;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return invalid;
        }
        else
        {
            req.headers.push_back(header());
            req.headers.back().name.push_back(input);
            state_ = header_name;
            return indeterminate;
        }
    case header_lws:
        if (input == '\r')
        {
            state_ = expecting_newline_2;
            return indeterminate;
        }
        else if (input == ' ' || input == '\t')
        {
            return indeterminate;
        }
        else if (is_ctl(input))
        {
            return invalid;
        }
        else
        {
            state_ = header_value;
            req.headers.back().value.push_back(input);
            return indeterminate;
        }
    case header_name:
        if (input == ':')
        {
            state_ = space_before_header_value;
            return indeterminate;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return invalid;
        }
        else
        {
            req.headers.back().name.push_back(input);
            return indeterminate;
        }
    case space_before_header_value:
        if (input == ' ')
        {
            state_ = header_value;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case header_value:
        if (input == '\r')
        {
            state_ = expecting_newline_2;
            return indeterminate;
        }
        else if (is_ctl(input))
        {
            return invalid;
        }
        else
        {
            req.headers.back().value.push_back(input);
            return indeterminate;
        }
    case expecting_newline_2:
        if (input == '\n')
        {
            state_ = header_line_start;
            return indeterminate;
        }
        else
        {
            return invalid;
        }
    case expecting_newline_3:
        return (input == '\n') ? valid : invalid;
    default:
        return invalid;
    }
}

bool request_parser::is_char(int c)
{
    return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
    return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
    switch (c)
    {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
        return true;
    default:
        return false;
    }
}

bool request_parser::is_digit(int c)
{
    return c >= '0' && c <= '9';
}