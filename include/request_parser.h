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
#include <tuple>
#include "request_parser.h"
#include "http_header.h"

class request_parser
{
    // parse a buffer of input from session into request
    // and store it in the request object
public:
    request_parser();
    enum result_type
    {
        valid,
        invalid,
        indeterminate
    };
    /// Parse some data. The enum return value is good when a complete request has
    /// been parsed, bad if the data is invalid, indeterminate when more data is
    /// required. The InputIterator return value indicates how much of the input
    /// has been consumed.
    template <typename InputIterator>
    std::tuple<result_type, InputIterator> parse(request &req,
                                                 InputIterator begin, InputIterator end)
    {
        while (begin != end)
        {
            result_type result = consume(req, *begin++);
            if (result == valid || result == invalid)
                return std::make_tuple(result, begin);
        }
        return std::make_tuple(indeterminate, begin);
    }

    int get_state() const
    {
        return state_;
    }

private:
    /// Handle the next character of input.
    result_type consume(request &req, char input);

    /// Check if a byte is an HTTP character.
    static bool is_char(int c);

    /// Check if a byte is an HTTP control character.
    static bool is_ctl(int c);

    /// Check if a byte is defined as an HTTP tspecial character.
    static bool is_tspecial(int c);

    /// Check if a byte is a digit.
    static bool is_digit(int c);

    /// The current state of the parser.
    // pretty self explanatory
    enum state
    {
        method_start, 
        method,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_start,
        http_version,
        http_version_end,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } state_;
};

#endif
