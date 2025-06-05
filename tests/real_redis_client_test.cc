#include "real_redis_client.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

class RealRedisClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Use default local Redis configuration
    client = std::make_unique<RealRedisClient>("127.0.0.1", 6379, 12);
  }

  void TearDown() override {
    client.reset();
  }

  std::unique_ptr<RealRedisClient> client;
};

// Test successful connection and ping
TEST_F(RealRedisClientTest, ConnectsSuccessfully) {
  // If we get here, connection was successful
  // The constructor would have called exit(1) if connection failed
  EXPECT_TRUE(true);
}

// Test basic SET and GET operations
TEST_F(RealRedisClientTest, SetAndGetWorkCorrectly) {
  const std::string key = "test_key";
  const std::string value = "test_value";

  // Set a value
  client->set(key, value);

  // Get the value back
  auto result = client->get(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), value);
}

// Test GET for non-existent key
TEST_F(RealRedisClientTest, GetNonExistentKeyReturnsNullopt) {
  auto result = client->get("non_existent_key");
  EXPECT_FALSE(result.has_value());
}

// Test SET overwrites existing value
TEST_F(RealRedisClientTest, SetOverwritesExistingValue) {
  const std::string key = "overwrite_key";
  const std::string value1 = "first_value";
  const std::string value2 = "second_value";

  // Set initial value
  client->set(key, value1);
  
  // Overwrite with new value
  client->set(key, value2);

  // Verify new value is stored
  auto result = client->get(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), value2);
}

// Test SET with empty value
TEST_F(RealRedisClientTest, SetEmptyValue) {
  const std::string key = "empty_key";
  const std::string value = "";

  client->set(key, value);

  auto result = client->get(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), value);
}

// Test SET with special characters
TEST_F(RealRedisClientTest, SetSpecialCharacters) {
  const std::string key = "special_key";
  const std::string value = "!@#$%^&*()_+{}|:\"<>?[]\\;',./";

  client->set(key, value);

  auto result = client->get(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), value);
}

// Test connection failure
TEST_F(RealRedisClientTest, ConnectionFailureExits) {
  EXPECT_EXIT(
      {
        // Try to connect to non-existent Redis server
        RealRedisClient bad_client("127.0.0.1", 9999, 12);
      },
      ::testing::ExitedWithCode(1),
      ""  // no regex on stderr
  );
} 