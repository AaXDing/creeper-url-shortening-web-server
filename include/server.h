// server.h
#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>

#include "config_parser.h"  // for NginxConfig
#include "isession.h"
#include "request_handler_dispatcher.h"  // for RequestHandler

using boost::asio::ip::tcp;
using SessionPtr = std::shared_ptr<ISession>;
using SessionFactory = std::function<SessionPtr()>;

class Server {
 public:
  Server(boost::asio::io_service& io, short port, const NginxConfig& config,
         SessionFactory fac = nullptr);
  
  friend class ServerTest;  

 private:
  void start_accept();
  void handle_accept(SessionPtr sess, const boost::system::error_code& ec);

  boost::asio::io_service& io_;
  tcp::acceptor acceptor_;
  SessionFactory make_session_;
  std::shared_ptr<RequestHandlerDispatcher> dispatcher_;
};

#endif  // SERVER_H
