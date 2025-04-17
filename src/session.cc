// A session class that handles reading and writing data to a client.
//
// Adopted from the Boost Asio example: async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include "session.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include "request_parser.h"
#include "http_header.h"

using boost::asio::ip::tcp;

session::session(boost::asio::io_service &io_service)
    : socket_(io_service)
{
}

tcp::socket &session::socket()
{
    return socket_;
}

void session::start()
{
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            boost::bind(&session::handle_read, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code &error,
                          size_t bytes_transferred)
{
    if (!error)
    {
        std::string request_msg(data_, bytes_transferred);
        request_parser p;
        request req;

        p.parse(req, request_msg);

        if (req.valid)
        {
            req.valid = true;
        }
        else
        {
            req.valid = false;
        }
        
        bool valid = req.valid;                  
        std::string version = req.valid ? req.version : HTTP_VERSION;
        std::string response_msg = handle_echo_response(version, request_msg, valid);

        // move response_msg to data_
        std::copy(response_msg.begin(), response_msg.end(), data_);
        size_t response_length = response_msg.size();
        // send response and continue reading loop
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(data_, response_length),
                                 boost::bind(&session::handle_write, this,
                                             boost::asio::placeholders::error));
    }
    else
    {
        delete this;
    }
}

void session::handle_write(const boost::system::error_code &error)
{
    if (!error)
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&session::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        delete this;
    }
}

// Constructs an HTTP response that echoes the provided request_msg.
// If valid is true, returns a 200 OK response echoing the original request;
// otherwise, returns a 400 Bad Request response.
std::string session::handle_echo_response(const std::string &http_version,
                        const std::string &request_msg,
                        bool valid)
{
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