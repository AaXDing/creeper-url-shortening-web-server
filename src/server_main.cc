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
#include "logging.h"
#include "server.h"

int main(int argc, char* argv[]) {
  logging::init_logging();
  try {
    if (argc != 2) {
      LOG(error) << "Wrong invocation, need exactly one argument";
      throw std::runtime_error("Usage: server <config_file>");
    }

    boost::asio::io_service io_service;

    LOG(info) << "Starting server with config file: " << argv[1];

    NginxConfig config;
    NginxConfigParser parser;

    if (!parser.parse(argv[1], &config)) {
      LOG(error) << "Error parsing config file: " << argv[1];
      throw std::runtime_error("Error parsing config file");
    }

    int port = config.get_port();
    if (port == -1) {
      LOG(error) << "No valid port found in config file";
      throw std::runtime_error("No valid port found in config file");
    }

    LOG(info) << "Creating server on port " << port;
    Server s(io_service, port, config);

    io_service.run();
  } catch (std::exception& e) {
    LOG(fatal) << "Uncaught exception: " << e.what();
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
