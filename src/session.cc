// A Session class that handles reading and writing data to a client.
//
// Adopted from the Boost Asio example: async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "session.h"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>

#include "http_header.h"
#include "request_handler_echo.h"
#include "request_parser.h"

using boost::asio::ip::tcp;

Session::Session(boost::asio::io_service &io_service) : socket_(io_service) {}

tcp::socket &Session::socket() { return socket_; }

void Session::start() {
  socket_.async_read_some(
      boost::asio::buffer(data_, MAX_LENGTH),
      boost::bind(&Session::handle_read, this, boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void Session::handle_read(const boost::system::error_code &error,
                          size_t bytes_transferred) {
  if (!error) {
    std::string response_msg = handle_response(bytes_transferred);

    // move response_msg to data_
    std::copy(response_msg.begin(), response_msg.end(), data_);
    size_t response_length = response_msg.size();
    // send Response and continue reading loop
    boost::asio::async_write(socket_,
                             boost::asio::buffer(data_, response_length),
                             boost::bind(&Session::handle_write, this,
                                         boost::asio::placeholders::error));
  } else {
    delete this;
  }
}

void Session::handle_write(const boost::system::error_code &error) {
  if (!error) {
    socket_.async_read_some(
        boost::asio::buffer(data_, MAX_LENGTH),
        boost::bind(&Session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else {
    delete this;
  }
}

// Parse the Request message and determine its validity.
// Constructs an HTTP Response that echoes the provided request_msg.
// If valid is true, returns a 200 OK Response echoing the original Request;
// otherwise, returns a 400 Bad Request Response.
std::string Session::handle_response(size_t bytes_transferred) {
  std::string request_msg(data_, bytes_transferred);
  RequestParser p;
  RequestHandlerEcho h;
  Request req;
  Response res;

  p.parse(req, request_msg);

  // TODO: distinguish between echo and static file req

  h.handle_request(req, res);

  std::string response_str = h.response_to_string(res);

  return response_str;
}