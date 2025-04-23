// tests/server_test.cpp

#include "server.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isession.h"
#include "session.h"  // for dynamic_cast<> on real session

using ::testing::_;
using ::testing::NiceMock;
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

 private:
  tcp::socket sock_;
};

// TEST 1: the normal success‐path with a custom factory
TEST(ServerTest, HandleAccept_CallsStartOnSuccess) {
  asio::io_service ios;
  auto mock = std::make_shared<NiceMock<MockSession>>(ios);

  // factory that always returns the same pointer
  ServerTest::SessionFactory factory = [mock]() {
    return std::unique_ptr<ISession>(mock.get());
  };

  ServerTest srv(ios, /*port=*/0, factory);

  EXPECT_CALL(*mock, start()).Times(1);

  error_code ec;  // success
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

  ServerTest srv(ios, /*port=*/0, factory);
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
  ServerTest srv(ios, /*port=*/0);

  // should run std::make_unique< Session >(io_) once
  EXPECT_NO_THROW(srv.call_start_accept());
}

// TEST 3b: directly capture what the default factory makes
TEST(ServerTest, DefaultFactory_ReturnsRealSession) {
  asio::io_service ios;
  ServerTest srv(ios, /*port=*/0);

  ISession* raw = srv.capture_session();
  // it really should be our concrete `Session` type:
  EXPECT_NE(dynamic_cast<Session*>(raw), nullptr)
      << "The default factory must produce a Session";
  delete raw;
}
