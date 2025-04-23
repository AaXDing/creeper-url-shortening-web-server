// Usage: ./server config_file
//
// Adopted from the Boost Asio example: async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>

#include "config_parser.h"
#include "server.h"

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      throw std::runtime_error("Usage: server <config_file>");
    }

    boost::asio::io_service io_service;

    NginxConfig config;
    NginxConfigParser parser;

    if (!parser.parse(argv[1], &config)) {
      throw std::runtime_error("Error parsing config file");
    }

    int port = config.get_port();
    if (port == -1) {
      throw std::runtime_error("No valid port found in config file");
    }
    Server s(io_service, port);

    io_service.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
