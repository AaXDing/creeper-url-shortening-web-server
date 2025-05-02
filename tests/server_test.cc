// tests/server_test.cc --------------------------------------------------
#include "server.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "config_parser.h"  // for NginxConfig
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isession.h"
#include "session.h"  // for dynamic_cast<> on real session

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace asio = boost::asio;
namespace sys  = boost::system;
using tcp        = asio::ip::tcp;
using error_code = sys::error_code;

// ------------------------------------------------ 1. Test-only wrapper
class ServerTest : public Server {
 public:
  using Server::Server;              // inherit ctor

  void call_start_accept() { start_accept(); }

  void call_handle_accept(SessionPtr sess, const error_code& ec) {
    handle_accept(std::move(sess), ec);
  }

  SessionPtr capture_session() { return make_session_(); }
};

// ------------------------------------------------ 2. Mock session
class MockSession : public ISession {
 public:
  explicit MockSession(asio::io_service& io) : sock_(io) {
    ON_CALL(*this, socket()).WillByDefault(ReturnRef(sock_));
  }

  MOCK_METHOD(tcp::socket&,   socket,         (), (override));
  MOCK_METHOD(void,           start,          (), (override));
  MOCK_METHOD(tcp::endpoint,  remote_endpoint,(), (override));

 private:
  tcp::socket sock_;
};

// ------------------------------------------------ 3. “success” path
TEST(ServerTest, HandleAccept_CallsStartOnSuccess) {
  asio::io_service ios;
  auto mock = std::make_shared<NiceMock<MockSession>>(ios);

  EXPECT_CALL(*mock, remote_endpoint())
      .WillOnce(Return(tcp::endpoint{
          asio::ip::address::from_string("127.0.0.1"), 4242}));

  SessionFactory factory = [mock]() { return mock; };   // shared_ptr copy

  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config, factory);

  EXPECT_CALL(*mock, start()).Times(1);

  error_code ec;                    // success
  srv.call_handle_accept(mock, ec); // pass shared_ptr
}

// ------------------------------------------------ 4. error path (no crash)
TEST(ServerTest, HandleAccept_Error_NoCrash) {
  asio::io_service ios;

  bool first = true;
  auto sess1 = std::make_shared<NiceMock<MockSession>>(ios);
  std::shared_ptr<NiceMock<MockSession>> sess2;

  SessionFactory factory = [&] {
    if (first) {
      first = false;
      return std::static_pointer_cast<ISession>(sess1);
    } else {
      sess2 = std::make_shared<NiceMock<MockSession>>(ios);
      return std::static_pointer_cast<ISession>(sess2);
    }
  };

  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config, factory);

  error_code ec = asio::error::operation_aborted;  // simulate failure
  EXPECT_NO_THROW(srv.call_handle_accept(sess1, ec)); // should not crash
}

// ------------------------------------------------ 5a. default factory via accept()
TEST(ServerTest, DefaultFactory_NoCrash_StartAccept) {
  asio::io_service ios;
  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config);

  EXPECT_NO_THROW(srv.call_start_accept());
}

// ------------------------------------------------ 5b. default factory product type
TEST(ServerTest, DefaultFactory_ProducesSession) {
  asio::io_service ios;
  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config);

  SessionPtr sp = srv.capture_session();
  EXPECT_NE(dynamic_cast<Session*>(sp.get()), nullptr)
      << "Default factory must create a concrete Session";
}
