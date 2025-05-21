// tests/session_test.cc -------------------------------------------------
#include "session.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <string>

#include "config_parser.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "request_handler_dispatcher.h"

using ::testing::AtLeast;

namespace asio = boost::asio;
namespace sys = boost::system;
using tcp = asio::ip::tcp;
using error_code = sys::error_code;

// ---------------------------------------------------------------- 1. Testable
// Session
class SessionTest : public Session {
 public:
  explicit SessionTest(asio::io_service& io)
      : Session(io, std::make_shared<RequestHandlerDispatcher>(NginxConfig())),
        deleted_flag_(nullptr) {
    // re-initialise dispatcher with an actual config file
    NginxConfig cfg;
    NginxConfigParser().parse("../my_config", &cfg);
    dispatcher_ = std::make_shared<RequestHandlerDispatcher>(cfg);
  }

  ~SessionTest() override {
    if (deleted_flag_) *deleted_flag_ = true;
  }

  // Override remote_endpoint to return a test-friendly endpoint
  tcp::endpoint remote_endpoint() override {
    return tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 4242);
  }

  // helpers to reach the protected/private bits
  std::string call_handle_response(std::size_t n) {
    return Session::handle_response(n);
  }
  void call_handle_read(const error_code& ec, std::size_t n) {
    Session::handle_read(ec, n);
  }
  void call_handle_write(const error_code& ec) { Session::handle_write(ec); }

  void set_data(const std::string& s) {
    std::copy(s.begin(), s.end(), data_);
    data_[s.size()] = '\0';
  }

  bool* deleted_flag_;
};

// ---------------------------------------------------------------- 2. Fixture
class SessionTestFixture : public ::testing::Test {
 protected:
  asio::io_service io;
  std::string http_version = "HTTP/1.1";
  std::string input;
  std::shared_ptr<SessionTest> sess = std::make_shared<SessionTest>(io);
};

// ---------------------------------------------------------------- 3. Response
// logic
TEST_F(SessionTestFixture, ValidRequestReturns200) {
  input =
      "GET /echo HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "\r\n";
  sess->set_data(input);

  std::string expected =
      http_version + " 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " +
      std::to_string(input.size()) + "\r\n\r\n" + input;

  EXPECT_EQ(sess->call_handle_response(input.size()), expected);
}

TEST_F(SessionTestFixture, InvalidRequestReturns400) {
  input =
      " /weird HTTP/1.1\r\n"
      "Host: weird.com\r\n"
      "\r\n";
  sess->set_data(input);

  std::string body = "400 Bad Request";
  std::string expected = http_version +
                         " 400 Bad Request\r\nContent-Type: text/plain\r\n"
                         "Content-Length: " +
                         std::to_string(body.size()) + "\r\n\r\n" + body;

  EXPECT_EQ(sess->call_handle_response(input.size()), expected);
}

TEST_F(SessionTestFixture, InvalidLocationReturns404) {
  input =
      "GET /nonexistent HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "\r\n";
  sess->set_data(input);

  std::string body = "404 Not Found";
  std::string expected = http_version +
                         " 404 Not Found\r\nContent-Type: text/plain\r\n"
                         "Content-Length: " +
                         std::to_string(body.size()) + "\r\n\r\n" + body;

  EXPECT_EQ(sess->call_handle_response(input.size()), expected);
}

// ------------------------------------------------------ 4. Read / write
// handlers Success path – object should remain alive.
TEST_F(SessionTestFixture, HandleReadSuccessKeepsSessionAlive) {
  input =
      "GET /index.html HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "\r\n";
  sess->set_data(input);

  bool deleted = false;
  sess->deleted_flag_ = &deleted;

  error_code ok;  // success
  sess->call_handle_read(ok, input.size());

  EXPECT_FALSE(deleted);
  sess->deleted_flag_ = nullptr;
}

TEST_F(SessionTestFixture, HandleWriteSuccessKeepsSessionAlive) {
  bool deleted = false;
  sess->deleted_flag_ = &deleted;

  error_code ok;
  sess->call_handle_write(ok);

  EXPECT_FALSE(deleted);
  sess->deleted_flag_ = nullptr;
}

// Error paths – destroy only when the last shared_ptr is gone.
static void expect_destruction_after(const std::function<void()>& invoke) {
  asio::io_service io;
  auto s = std::make_shared<SessionTest>(io);
  bool deleted = false;
  s->deleted_flag_ = &deleted;
  std::weak_ptr<SessionTest> weak = s;

  invoke();  // run the handler while `s` is still held

  s.reset();  // drop the last strong ref
  EXPECT_TRUE(deleted);
  EXPECT_TRUE(weak.expired());
}

TEST_F(SessionTestFixture, HandleReadErrorDestroysSession) {
  expect_destruction_after([&] {
    error_code ec = asio::error::eof;
    sess->call_handle_read(ec, 0);
  });
}

TEST_F(SessionTestFixture, HandleWriteErrorDestroysSession) {
  expect_destruction_after([&] {
    error_code ec = asio::error::eof;
    sess->call_handle_write(ec);
  });
}

// ---------------------------------------------------------------- 5. start()
TEST_F(SessionTestFixture, StartDoesNotDestroySessionImmediately) {
  bool deleted = false;
  sess->deleted_flag_ = &deleted;
  sess->start();
  EXPECT_FALSE(deleted);
  sess->deleted_flag_ = nullptr;
}
