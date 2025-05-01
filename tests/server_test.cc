// tests/server_test.cpp

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
namespace sys = boost::system;
using tcp = asio::ip::tcp;
using error_code = sys::error_code;

// Test‑only helper: inherits the ctor, then exposes the two privates.
class ServerTest : public Server {
 public:
  using Server::Server;  // inherit constructor

  // Expose private start_accept()
  void call_start_accept() { start_accept(); }

  // Expose private handle_accept(...)
  void call_handle_accept(ISession* sess, const boost::system::error_code& ec) {
    handle_accept(sess, ec);
  }

  // directly run the factory and return the raw ISession* it creates
  ISession* capture_session() {
    auto up = make_session_();
    return up.release();
  }
};

// A mock ISession that carries a real socket so async_accept can bind.
class MockSession : public ISession {
 public:
  explicit MockSession(asio::io_service& io) : sock_(io) {
    ON_CALL(*this, socket()).WillByDefault(ReturnRef(sock_));
  }

  MOCK_METHOD(tcp::socket&, socket, (), (override));
  MOCK_METHOD(void, start, (), (override));
  MOCK_METHOD(tcp::endpoint, remote_endpoint, (), (override));

 private:
  tcp::socket sock_;
};

// TEST 1: original “success” path, with a real accept/connect
TEST(ServerTest, HandleAccept_CallsStartOnSuccess) {
  asio::io_service ios;
  auto mock = std::make_shared<NiceMock<MockSession>>(ios);

  EXPECT_CALL(*mock, remote_endpoint())
      .WillOnce(Return(
          tcp::endpoint{asio::ip::address::from_string("127.0.0.1"), 4242}));

  // Now factory + server
  ServerTest::SessionFactory factory = [mock]() {
    return std::unique_ptr<ISession>(mock.get());
  };
  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config, factory);

  // Expect start() to be invoked once on success
  EXPECT_CALL(*mock, start()).Times(1);

  // Call handle_accept; since remote_endpoint() now works, it won't throw
  error_code ec;  // ec == success
  srv.call_handle_accept(mock.get(), ec);
}

// TEST 2: the error‐path, ensuring we don’t crash when delete + new accept()
TEST(ServerTest, HandleAccept_DeletesOnError_NoCrash) {
  asio::io_service ios;

  bool first = true;
  NiceMock<MockSession>* raw1 = new NiceMock<MockSession>(ios);
  NiceMock<MockSession>* raw2 = nullptr;

  ServerTest::SessionFactory factory = [&] {
    if (first) {
      first = false;
      return std::unique_ptr<ISession>(raw1);
    } else {
      raw2 = new NiceMock<MockSession>(ios);
      return std::unique_ptr<ISession>(raw2);
    }
  };

  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config, factory);
  error_code ec = asio::error::operation_aborted;
  srv.call_handle_accept(raw1, ec);

  // handle_accept deleted raw1, but raw2 was never deleted by server…
  delete raw2;  // <— clean it up here
  SUCCEED() << "error-path ran without crashing";
}

// TEST 3a: fire the default factory via start_accept()
TEST(ServerTest, DefaultFactory_DoesNotCrash_StartAccept) {
  asio::io_service ios;

  // no factory ⇒ default branch is used
  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config);

  // should run std::make_unique< Session >(io_) once
  EXPECT_NO_THROW(srv.call_start_accept());
}

// TEST 3b: directly capture what the default factory makes
TEST(ServerTest, DefaultFactory_ReturnsRealSession) {
  asio::io_service ios;

  NginxConfig config;
  ServerTest srv(ios, /*port=*/0, config);

  ISession* raw = srv.capture_session();
  // it really should be our concrete `Session` type:
  EXPECT_NE(dynamic_cast<Session*>(raw), nullptr)
      << "The default factory must produce a Session";
  delete raw;
}
