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
        auto res = p.parse(req, data_, data_ + bytes_transferred);
        if (std::get<0>(res) == request_parser::valid)
        {
            req.valid = true;
        }
        else
        {
            req.valid = false;
        }
        std::string response_msg;
        if (req.valid)
        {
            response res;
            res.version = req.version;
            res.status_code = 200;
            res.status_message = "OK";
            res.content_type = "text/html";
            res.body = request_msg; // echo back the request (might change based on future needs)
            response_msg = res.version + " " +
                           std::to_string(res.status_code) + " " +
                           res.status_message + "\r\n" +
                           "Content-Type: " + res.content_type + "\r\n" +
                           "Content-Length: " + std::to_string(res.body.size()) + "\r\n" +
                           "\r\n" +
                           res.body;
        }
        else
        {
            response res;
            res.version = HTTP_VERSION;
            res.status_code = 400;
            res.status_message = "Bad Request";
            res.content_type = "text/plain";
            res.body = "400 Bad Request";
            response_msg = res.version + " " +
                           std::to_string(res.status_code) + " " +
                           res.status_message + "\r\n" +
                           "Content-Type: " + res.content_type + "\r\n" +
                           "Content-Length: " + std::to_string(res.body.size()) + "\r\n" +
                           "\r\n" +
                           res.body;
        }
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