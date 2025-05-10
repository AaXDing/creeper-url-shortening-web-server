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
#include <memory>
#include <string>

#include "echo_request_handler.h"
#include "http_header.h"
#include "logging.h"
#include "request_handler_dispatcher.h"
#include "request_parser.h"

using boost::asio::ip::tcp;
using boost::asio::placeholders::bytes_transferred;
using boost::asio::placeholders::error;

Session::Session(boost::asio::io_service &io_service,
                 std::shared_ptr<RequestHandlerDispatcher> dispatcher)
    : socket_(io_service), dispatcher_(dispatcher) {}

tcp::socket &Session::socket() { return socket_; }

void Session::start() {
  auto self = shared_from_this();
  socket_.async_read_some(
      boost::asio::buffer(data_, MAX_LENGTH),
      boost::bind(&Session::handle_read, self, error, bytes_transferred));
}

void Session::handle_read(const boost::system::error_code &error,
                          size_t bytes_transferred) {
  if (!error) {
    std::string response_msg = handle_response(bytes_transferred);

    size_t response_length = response_msg.size();
    // send Response and continue reading loop

    auto self = shared_from_this();  // keep-alive again
    boost::asio::async_write(socket_, boost::asio::buffer(response_msg),
                             boost::bind(&Session::handle_write, self,
                                         boost::asio::placeholders::error));

  } else if (error == boost::asio::error::eof ||
             error == boost::asio::error::connection_reset) {
    // client closed connection normally
    LOG(info) << "Client disconnected: " << error.message();
  } else {
    // truly unexpected
    LOG(error) << "Read error: " << error.message();
  }
}

void Session::handle_write(const boost::system::error_code &error) {
  if (!error) {
    auto self = shared_from_this();  // keep-alive
    socket_.async_read_some(
        boost::asio::buffer(data_, MAX_LENGTH),
        boost::bind(&Session::handle_read, self,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else if (error == boost::asio::error::eof ||
             error == boost::asio::error::connection_reset) {
    LOG(info) << "Client disconnected during write: " << error.message();
  } else {
    LOG(error) << "Write error: " << error.message();
  }
}

// Parse the Request message and determine its validity.
// Constructs an HTTP Response that echoes the provided request_msg.
// If valid is true, returns a 200 OK Response echoing the original Request;
// otherwise, returns a 400 Bad Request Response.
// RequestHandlerDispatcher will base the parsing to generate the specific
// handler
std::string Session::handle_response(size_t bytes_transferred) {
  std::string request_msg(data_, bytes_transferred);
  RequestParser p;
  Request req;
  std::unique_ptr<Response> res;

  p.parse(req, request_msg);
  if (!req.valid) {
    // If the request is invalid, return a 400 Bad Request response
    LOG(warning) << "Invalid request → 400";
    return STOCK_RESPONSE.at(400).to_string();
  }

  std::unique_ptr<RequestHandler> h = dispatcher_->get_handler(req);

  if (h != nullptr) {
    LOG(info) << "Dispatching to handler for uri=" << req.uri;
    res = h->handle_request(req);
  } else {
    // If no handler is found, return a 404 Not Found response
    LOG(warning) << "No handler for uri=" << req.uri << " → 404";
    res = std::make_unique<Response>(STOCK_RESPONSE.at(404));
  }

  return res->to_string();
}