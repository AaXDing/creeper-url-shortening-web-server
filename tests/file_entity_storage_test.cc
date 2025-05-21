#include "file_entity_storage.h"

#include <filesystem>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class FileEntityStorageTest : public ::testing::Test {
protected:
  std::string storage_root = "./test_storage_env";

  void SetUp() override {
    fs::remove_all(storage_root);
    fs::create_directories(storage_root);
  }

  void TearDown() override { fs::remove_all(storage_root); }
};

TEST_F(FileEntityStorageTest, CanCreateAndReadEntityBack) {
  FileEntityStorage backend(storage_root);
  auto result_id = backend.create("Books", "{\"title\":\"CS130\"}");

  ASSERT_TRUE(result_id.has_value());
  auto fetched = backend.retrieve("Books", result_id.value());

  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(fetched.value(), "{\"title\":\"CS130\"}");
}

TEST_F(FileEntityStorageTest, OverwritesEntityDataOnUpdate) {
  FileEntityStorage backend(storage_root);
  int id = backend.create("Notes", "draft").value();

  ASSERT_TRUE(backend.update("Notes", id, "final"));

  auto result = backend.retrieve("Notes", id);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "final");
}

TEST_F(FileEntityStorageTest, RemoveEntityMakesItUnretrievable) {
  FileEntityStorage backend(storage_root);
  int id = backend.create("Shoes", "nike").value();

  EXPECT_TRUE(backend.remove("Shoes", id));

  auto result = backend.retrieve("Shoes", id);
  EXPECT_FALSE(result.has_value());
}

TEST_F(FileEntityStorageTest, ListsAllEntityIds) {
  FileEntityStorage backend(storage_root);
  backend.create("Cars", "Honda");
  backend.create("Cars", "Toyota");

  std::vector<int> found_ids = backend.list("Cars");

  EXPECT_EQ(found_ids.size(), 2);
  EXPECT_NE(std::find(found_ids.begin(), found_ids.end(), 1), found_ids.end());
  EXPECT_NE(std::find(found_ids.begin(), found_ids.end(), 2), found_ids.end());
}

TEST_F(FileEntityStorageTest, UpdateOnMissingIdCreatesIt) {
  FileEntityStorage backend(storage_root);

  // Simulate PUT to non-existent ID
  EXPECT_TRUE(backend.update("Magazines", 42, "Special Edition"));

  auto fetched = backend.retrieve("Magazines", 42);
  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(fetched.value(), "Special Edition");
}
