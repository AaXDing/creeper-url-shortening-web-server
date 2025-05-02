// server.cc ---------------------------------------------------------------
#include "server.h"

#include <boost/bind/bind.hpp>
#include <boost/asio/placeholders.hpp>

#include "config_parser.h"
#include "logging.h"
#include "session.h"  // default factory creates this concrete type

using boost::asio::ip::tcp;
using boost::asio::placeholders::error;

// ------------------------------------------------------------------ ctor
Server::Server(boost::asio::io_service& io, short port,
               const NginxConfig& config, SessionFactory factory)
    : io_(io),
      acceptor_(io, tcp::endpoint(tcp::v4(), port)),
      dispatcher_(std::make_shared<RequestHandlerDispatcher>(config)),
      make_session_(factory
                        ? std::move(factory)  // test / mock
                        : [&] {               // default
                            return std::make_shared<Session>(io_, dispatcher_);
                          }) {
  LOG(info) << "Server listening on port " << port;
  start_accept();
}

// --------------------------------------------------------- start_accept()
void Server::start_accept() {
  LOG(info) << "Waiting for new connection…";

  // Produce a shared_ptr<ISession>
  SessionPtr session = make_session_();

  acceptor_.async_accept(session->socket(),                   // tcp::socket&
                         boost::bind(&Server::handle_accept,  // member fn
                                     this,                    // Server*
                                     session,                 // keep session alive
                                     error));
}

// --------------------------------------------------------- handle_accept()
void Server::handle_accept(SessionPtr sess,
                           const boost::system::error_code& ec) {
  if (!ec) {
    tcp::endpoint ep = sess->remote_endpoint();
    LOG(info) << "Accepted connection from " << ep.address().to_string() << ':'
              << ep.port();
    sess->start();
  } else {
    LOG(error) << "Accept error: " << ec.message();
  }

  start_accept();  // wait for next client
}
