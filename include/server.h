// server.h
#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include "isession.h"

using boost::asio::ip::tcp;

class server {
public:
    // Factory type: returns a new ISession ready for accept().
    using SessionFactory = std::function<std::unique_ptr<ISession>()>;

    // Construct on given io_service and port, with optional custom factory.
    server(boost::asio::io_service& io,
           short port,
           SessionFactory fac = nullptr);

    // Allow our test helper Serv to reach private methods.
    friend class Serv;

private:
    void start_accept();
    void handle_accept(ISession* sess,
                       const boost::system::error_code& ec);

    boost::asio::io_service& io_;
    tcp::acceptor            acceptor_;
    SessionFactory           make_session_;
};

#endif // SERVER_H
