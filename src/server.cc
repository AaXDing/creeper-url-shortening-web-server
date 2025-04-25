// A Server class that accepts incoming connections and starts a Session for
// each client.
//
// Adopted from the Boost Asio example: async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "server.h"

#include <boost/bind/bind.hpp>

#include "logging.h"
#include "session.h"  // only for the default factory

using boost::asio::ip::tcp;

Server::Server(boost::asio::io_service &io, short port, SessionFactory factory)
    : io_(io),
      acceptor_(io, tcp::endpoint(tcp::v4(), port)),
      make_session_(factory ? std::move(factory)  // test/mock path
                            : [&] {
                                return std::make_unique<Session>(io_);
                              }) {  // default path
  LOG(info) << "Server listening on port " << port;
  start_accept();
}

void Server::start_accept() {
  std::unique_ptr<ISession> next = make_session_();  // heap‑allocated

  ISession *raw = next.release();  // we will delete or transfer later

  acceptor_.async_accept(
      raw->socket(),                       // ISession exposes socket()
      boost::bind(&Server::handle_accept,  // completion handler
                  this, raw, boost::asio::placeholders::error));
}

void Server::handle_accept(ISession *sess,
                           const boost::system::error_code &ec) {
  if (!ec) {
    auto ep = sess->remote_endpoint();
    LOG(info) << "Accepted connection from " << ep.address().to_string() << ":"
              << ep.port();
    sess->start();  // connection successful – kick off session logic
  } else {
    delete sess;  // reclaim memory on failure
  }
  start_accept();  // immediately wait for the next client
}
