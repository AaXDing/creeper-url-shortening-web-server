#include "shorten_request_handler.h"

#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "gtest/gtest.h"
#include "idatabase_client.h"
#include "iredis_client.h"

// -----------------------------------------------------------------------------
// Fake implementations of IRedisClient and IDatabaseClient for unit testing.
// -----------------------------------------------------------------------------

class FakeRedisClient : public IRedisClient {
 public:
  FakeRedisClient() = default;

  std::optional<std::string> get(const std::string& short_code) override {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = store_.find(short_code);
    if (it == store_.end()) return std::nullopt;
    return it->second;
  }

  void set(const std::string& short_code,
           const std::string& long_url) override {
    std::lock_guard<std::mutex> lock(mu_);
    store_[short_code] = long_url;
  }

 private:
  std::unordered_map<std::string, std::string> store_;
  std::mutex mu_;
};

class FakeDatabaseClient : public IDatabaseClient {
 public:
  FakeDatabaseClient() = default;

  bool store(const std::string& short_code,
             const std::string& long_url) override {
    std::lock_guard<std::mutex> lock(mu_);
    store_[short_code] = long_url;  // upsert
    return true;
  }

  std::optional<std::string> lookup(const std::string& short_code) override {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = store_.find(short_code);
    if (it == store_.end()) return std::nullopt;
    return it->second;
  }

 private:
  std::unordered_map<std::string, std::string> store_;
  std::mutex mu_;
};

class AlwaysFailDB : public IDatabaseClient {
 public:
  AlwaysFailDB() = default;

  // Always return false to simulate a database‐store failure:
  bool store(const std::string& short_code,
             const std::string& long_url) override {
    return false;
  }

  // Not needed
  std::optional<std::string> lookup(const std::string&) override {
    return std::nullopt;
  }
};

// -----------------------------------------------------------------------------
// Helper functions to create minimal Request objects.
// -----------------------------------------------------------------------------

static Request make_post_request(const std::string& base_uri,
                                 const std::string& long_url) {
  Request req;
  req.method = "POST";
  req.uri = base_uri;
  req.version = "HTTP/1.1";
  req.body = long_url;
  req.valid = true;
  return req;
}

static Request make_get_request(const std::string& base_uri,
                                const std::string& short_code) {
  Request req;
  req.method = "GET";
  req.uri = base_uri + "/" + short_code;
  req.version = "HTTP/1.1";
  req.valid = true;
  return req;
}

static Request make_bad_method_request(const std::string& base_uri) {
  Request req;
  req.method = "DELETE";
  req.uri = base_uri;
  req.version = "HTTP/1.1";
  req.valid = true;
  return req;
}

// -----------------------------------------------------------------------------
// Test fixture for ShortenRequestHandler.
// -----------------------------------------------------------------------------

class ShortenHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    base_uri = "/shorten";
    args = std::make_shared<ShortenRequestHandlerArgs>();

    // Override the real clients with fakes
    fake_redis = std::make_shared<FakeRedisClient>();
    fake_db = std::make_shared<FakeDatabaseClient>();
    args->redis_client = fake_redis;
    args->db_client = fake_db;

    handler = std::make_unique<ShortenRequestHandler>(base_uri, args);
  }

  std::string base_uri;
  std::shared_ptr<ShortenRequestHandlerArgs> args;
  std::shared_ptr<FakeRedisClient> fake_redis;
  std::shared_ptr<FakeDatabaseClient> fake_db;
  std::unique_ptr<ShortenRequestHandler> handler;
};

// -----------------------------------------------------------------------------
//  1) POST should store in DB and return a 6-character code.
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, PostStoresInDatabaseAndReturnsCode) {
  const std::string long_url = "https://example.com/foo";

  auto post_req = make_post_request(base_uri, long_url);
  auto resp = handler->handle_request(post_req);

  // Expect 200 OK
  ASSERT_EQ(resp->status_code, 200);
  ASSERT_EQ(resp->status_message, "OK");

  // Body should be exactly 6 characters
  ASSERT_EQ(resp->body.size(), 6u);
  std::string code = resp->body;

  // Verify that DB got the mapping
  auto db_val = fake_db->lookup(code);
  ASSERT_TRUE(db_val.has_value());
  EXPECT_EQ(db_val.value(), long_url);

  // Redis should still be empty for that code
  auto redis_val = fake_redis->get(code);
  EXPECT_FALSE(redis_val.has_value());
}

// -----------------------------------------------------------------------------
//  2) GET when code is not in Redis but is in DB: returns 302 and caches in
//  Redis.
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, GetFallbackToDBAndCacheRedis) {
  // First, insert into DB manually (simulate a prior POST)
  const std::string code = "ABC123";
  const std::string long_url = "https://test.com/bar";
  fake_db->store(code, long_url);

  // Ensure Redis does not have it yet
  EXPECT_FALSE(fake_redis->get(code).has_value());

  // Now perform GET
  auto get_req = make_get_request(base_uri, code);
  auto resp = handler->handle_request(get_req);

  // Expect a 302 redirect
  ASSERT_EQ(resp->status_code, 302);
  ASSERT_EQ(resp->headers.size(), 1u);
  EXPECT_EQ(resp->headers[0].name, "Location");
  EXPECT_EQ(resp->headers[0].value, long_url);

  // Now Redis should have been populated
  auto redis_val = fake_redis->get(code);
  ASSERT_TRUE(redis_val.has_value());
  EXPECT_EQ(redis_val.value(), long_url);
}

// -----------------------------------------------------------------------------
//  3) GET when code is in Redis: returns 302 immediately, no DB lookup.
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, GetHitsRedisDirectly) {
  const std::string code = "XYZ789";
  const std::string long_url = "https://cached.example.com";

  // Pre-populate Redis only
  fake_redis->set(code, long_url);

  // Ensure DB does not have it
  EXPECT_FALSE(fake_db->lookup(code).has_value());

  // Now GET
  auto get_req = make_get_request(base_uri, code);
  auto resp = handler->handle_request(get_req);

  // Should redirect using Redis value
  ASSERT_EQ(resp->status_code, 302);
  ASSERT_EQ(resp->headers.size(), 1u);
  EXPECT_EQ(resp->headers[0].name, "Location");
  EXPECT_EQ(resp->headers[0].value, long_url);

  // DB should remain unchanged (still no entry)
  EXPECT_FALSE(fake_db->lookup(code).has_value());
}

// -----------------------------------------------------------------------------
//  4) GET with malformed URI (wrong length) should return 404
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, GetInvalidUriLengthReturns404) {
  // Too short (only 3 chars instead of 6)
  Request bad_short;
  bad_short.method = "GET";
  bad_short.uri = base_uri + "/ABC";
  bad_short.version = "HTTP/1.1";
  bad_short.valid = true;
  auto resp1 = handler->handle_request(bad_short);
  EXPECT_EQ(resp1->status_code, 404);

  // Too long (8 chars instead of 6)
  Request bad_long;
  bad_long.method = "GET";
  bad_long.uri = base_uri + "/TOOLONG1";
  bad_long.version = "HTTP/1.1";
  bad_long.valid = true;
  auto resp2 = handler->handle_request(bad_long);
  EXPECT_EQ(resp2->status_code, 404);
}

// -----------------------------------------------------------------------------
//  5) Collision handling: if two URLs hash to same code, second overwrites DB
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, PostCollisionUpsertsDatabase) {
  const std::string url1 = "https://collision.one";
  const std::string url2 = "https://collision.two";

  // POST first URL
  auto resp1 = handler->handle_request(make_post_request(base_uri, url1));
  std::string code1 = resp1->body;

  // POST second URL
  auto resp2 = handler->handle_request(make_post_request(base_uri, url2));
  std::string code2 = resp2->body;

  if (code1 == code2) {
    // If a collision occurred, DB should now have url2
    auto db_val = fake_db->lookup(code1);
    ASSERT_TRUE(db_val.has_value());
    EXPECT_EQ(db_val.value(), url2);
  } else {
    // No collision: each code maps to its own URL
    auto db_val1 = fake_db->lookup(code1);
    auto db_val2 = fake_db->lookup(code2);
    ASSERT_TRUE(db_val1.has_value());
    ASSERT_TRUE(db_val2.has_value());
    EXPECT_EQ(db_val1.value(), url1);
    EXPECT_EQ(db_val2.value(), url2);
  }
}

// -----------------------------------------------------------------------------
//  6) GET with no entry in Redis or DB → 404
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, GetMissingInBothReturns404) {
  const std::string code = "NOEXST";  // arbitrary 6-char code

  // Ensure neither Redis nor DB has it
  EXPECT_FALSE(fake_redis->get(code).has_value());
  EXPECT_FALSE(fake_db->lookup(code).has_value());

  // Now GET
  auto get_req = make_get_request(base_uri, code);
  auto resp = handler->handle_request(get_req);

  // Should return 404
  EXPECT_EQ(resp->status_code, 404);
}

// -----------------------------------------------------------------------------
//  7) Unsupported HTTP method should return 405
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, UnsupportedMethodReturns405) {
  auto bad_req = make_bad_method_request(base_uri);
  auto resp = handler->handle_request(bad_req);

  EXPECT_EQ(resp->status_code, 405);
}

// -----------------------------------------------------------------------------
//  8) get_type() should return the correct enum
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, GetTypeReturnsExpectedEnum) {
  EXPECT_EQ(handler->get_type(),
            RequestHandler::HandlerType::SHORTEN_REQUEST_HANDLER);
}

// -----------------------------------------------------------------------------
//  9) DB store failling should return 500
// -----------------------------------------------------------------------------
TEST_F(ShortenHandlerTest, WhenDbStoreFails_Returns500) {
  // Build args with a DB client that always returns false.
  auto args = std::make_shared<ShortenRequestHandlerArgs>();
  args->redis_client = std::make_shared<FakeRedisClient>();
  args->db_client = std::make_shared<AlwaysFailDB>();
  ShortenRequestHandler handler("/shorten", args);

  // Create a POST request and invoke handle_request(...).
  Request req = make_post_request("/shorten", "https://example.com/fail");
  auto resp = handler.handle_request(req);

  // Verify that we took the “store failed” branch:
  // status_code == 500, and the body is “Failed to store URL mapping”.
  EXPECT_EQ(resp->status_code, 500);
  EXPECT_EQ(resp->status_message, "Internal Server Error");
  EXPECT_EQ(resp->body, "Failed to store URL mapping");
}

//----------------------------------------------------------------------------‐
// 10) create_from_config() should return “inline‐fake” clients when the env var
// is set.
//----------------------------------------------------------------------------‐
TEST(ShortenHandlerArgsTest, CreateFromConfigReturnsInlineFakesWhenEnvSet) {
  // 1) Put the process into "fake‐mode"
  ASSERT_EQ(setenv("USE_FAKE_SHORTEN_CLIENTS", "1", /*overwrite=*/1), 0);

  // 2) Call create_from_config; the argument is ignored when the fake‐mode is
  // enabled.
  auto args = ShortenRequestHandlerArgs::create_from_config(nullptr);
  ASSERT_TRUE(args) << "Expected a non‐null ShortenRequestHandlerArgs";

  // 3) Grab the IRedisClient and IDatabaseClient pointers
  auto redis_ptr = args->redis_client;
  auto db_ptr = args->db_client;
  ASSERT_TRUE(redis_ptr) << "Expected redis_client to be set to a fake";
  ASSERT_TRUE(db_ptr) << "Expected db_client to be set to a fake";

  // 4) Verify “inline‐fake” Redis behavior:
  const std::string short_code = "ABCDEF";
  const std::string long_url = "https://fake.test/url";

  // Initially, fake get() should be empty
  std::optional<std::string> maybe_redis0 = redis_ptr->get(short_code);
  EXPECT_FALSE(maybe_redis0.has_value());

  // After set, get() should return what we stored
  redis_ptr->set(short_code, long_url);
  std::optional<std::string> maybe_redis1 = redis_ptr->get(short_code);
  ASSERT_TRUE(maybe_redis1.has_value());
  EXPECT_EQ(maybe_redis1.value(), long_url);

  // 5) Verify “inline‐fake” DB behavior:
  std::optional<std::string> maybe_db0 = db_ptr->lookup(short_code);
  EXPECT_FALSE(maybe_db0.has_value());

  bool ok_store = db_ptr->store(short_code, long_url);
  EXPECT_TRUE(ok_store);

  std::optional<std::string> maybe_db1 = db_ptr->lookup(short_code);
  ASSERT_TRUE(maybe_db1.has_value());
  EXPECT_EQ(maybe_db1.value(), long_url);

  // 6) Clean up the env var so other tests are unaffected:
  ASSERT_EQ(unsetenv("USE_FAKE_SHORTEN_CLIENTS"), 0);
}