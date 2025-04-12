// Usage: ./server <port>
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
#include <cstdlib>
#include <boost/asio.hpp>
#include "server.h"
#include "config_parser.h"

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    NginxConfig config;
    NginxConfigParser parser;

    if (!parser.Parse(argv[1], &config))
    {
      std::cerr << "Error parsing config file\n";
      return 1;
    }


    int port = config.getPort();
    if (port == -1)
    {
      std::cerr << "Error: No port specified in config file\n";
      return 1;
    }
    server s(io_service, port);

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
