// server.h
#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>

#include "isession.h"
#include "config_parser.h"  // for NginxConfig
#include "request_handler_dispatcher.h"  // for RequestHandler

using boost::asio::ip::tcp;

class Server {
 public:
  // Factory type: returns a new ISession ready for accept().
  using SessionFactory = std::function<std::unique_ptr<ISession>()>;

  // Construct on given io_service and port, with optional custom factory.
  Server(boost::asio::io_service& io, short port, const NginxConfig& config, SessionFactory fac = nullptr);

  // Allow our test helper ServerTest to reach private methods.
  friend class ServerTest;

 private:
  void start_accept();
  void handle_accept(ISession* sess, const boost::system::error_code& ec);

  boost::asio::io_service& io_;
  tcp::acceptor acceptor_;
  SessionFactory make_session_;
  std::shared_ptr<RequestHandlerDispatcher> dispatcher_;
};

#endif  // SERVER_H
