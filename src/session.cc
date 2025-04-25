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
#include "request_handler_dispatcher.h"
#include "request_parser.h"

using boost::asio::ip::tcp;

Session::Session(boost::asio::io_service &io_service,
                 std::shared_ptr<RequestHandlerDispatcher> dispatcher)
    : socket_(io_service), dispatcher_(dispatcher) {}

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
// RequestHandlerDispatcher will base the parsing to generate the specific
// handler
std::string Session::handle_response(size_t bytes_transferred) {
  std::string request_msg(data_, bytes_transferred);

  RequestParser p;
  Request req;
  Response res;

  p.parse(req, request_msg);

  auto h = dispatcher_->get_handler(req);
  std::string response_str;

  if (!req.valid){
    // If the request is invalid, return a 400 Bad Request response
    res.status_code = 400;
    res.status_message = "Bad Request";
    res.version = HTTP_VERSION;
    res.content_type = "text/plain";
    res.body = "400 Bad Request";

    response_str = res.version + " " + std::to_string(res.status_code) + " " +
                  res.status_message + "\r\n" +
                  "Content-Type: " + res.content_type + "\r\n" +
                  "Content-Length: " + std::to_string(res.body.size()) + "\r\n" +
                  "\r\n" + res.body;
  }
  else if (h != nullptr) {
    h->handle_request(req, res);
    response_str = h->response_to_string(res);
  } 
  else {
    // If no handler is found, return a 404 Not Found response
    res.status_code = 404;
    res.status_message = "Not Found";
    res.version = HTTP_VERSION;
    res.content_type = "text/plain";
    res.body = "404 Not Found";
    
    response_str = res.version + " " + std::to_string(res.status_code) + " " +
                  res.status_message + "\r\n" +
                  "Content-Type: " + res.content_type + "\r\n" +
                  "Content-Length: " + std::to_string(res.body.size()) + "\r\n" +
                  "\r\n" + res.body;
  }

  

  

  return response_str;
}