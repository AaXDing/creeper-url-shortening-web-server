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

// The following is a comment block that contains a stylized ASCII art
// adopted from https://gist.github.com/edokeh/7580064

//
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//               佛祖保佑         永无BUG
//
//
//