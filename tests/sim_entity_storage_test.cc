#include "sim_entity_storage.h"

#include "gtest/gtest.h"

class SimEntityStorageTest : public ::testing::Test {
 protected:
  SimEntityStorage backend;
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(SimEntityStorageTest, CanCreateAndReadEntityBack) {
  std::string resource = "test_resource";
  std::string data = R"({"name": "test", "value": 42})";

  auto id = backend.create(resource, data);
  ASSERT_TRUE(id.has_value());

  auto retrieved_data = backend.retrieve(resource, id.value());
  ASSERT_TRUE(retrieved_data.has_value());
  EXPECT_EQ(retrieved_data.value(), data);
}

TEST_F(SimEntityStorageTest, RetreiveNonExistentEntity) {
  std::string resource = "non_existent_resource";
  int id = 999;

  auto retrieved_data = backend.retrieve(resource, id);
  EXPECT_FALSE(retrieved_data.has_value());
}

TEST_F(SimEntityStorageTest, UpdateCreateAndUpdateEntity) {
  int id = 69;
  std::string resource = "update_resource";
  std::string initial_data = R"({"name": "initial", "value": 1})";
  std::string updated_data = R"({"name": "updated", "value": 2})";

  bool res = backend.update(resource, id, initial_data);
  ASSERT_TRUE(res);

  auto retrieved_data = backend.retrieve(resource, id);
  ASSERT_TRUE(retrieved_data.has_value());
  EXPECT_EQ(retrieved_data.value(), initial_data);

  EXPECT_TRUE(backend.update(resource, id, updated_data));

  retrieved_data = backend.retrieve(resource, id);
  ASSERT_TRUE(retrieved_data.has_value());
  EXPECT_EQ(retrieved_data.value(), updated_data);
}

TEST_F(SimEntityStorageTest, RemoveEntity) {
  std::string resource = "remove_resource";
  std::string data = R"({"name": "to_remove", "value": 3})";

  auto id = backend.create(resource, data);
  ASSERT_TRUE(id.has_value());

  EXPECT_TRUE(backend.remove(resource, id.value()));

  auto retrieved_data = backend.retrieve(resource, id.value());
  EXPECT_FALSE(retrieved_data.has_value());
}

TEST_F(SimEntityStorageTest, RemoveNonExistentEntity) {
  std::string resource = "non_existent_resource";
  int id = 999;

  EXPECT_FALSE(backend.remove(resource, id));
}

TEST_F(SimEntityStorageTest, ListEntities) {
  std::string resource = "list_resource";
  backend.create(resource, R"({"name": "item1", "value": 1})");
  backend.create(resource, R"({"name": "item2", "value": 2})");

  auto ids = backend.list(resource);
  EXPECT_EQ(ids.size(), 2);
  EXPECT_NE(std::find(ids.begin(), ids.end(), 1), ids.end());
  EXPECT_NE(std::find(ids.begin(), ids.end(), 2), ids.end());
}

TEST_F(SimEntityStorageTest, ListNonExistentResource) {
  std::string resource = "non_existent_resource";
  auto ids = backend.list(resource);
  EXPECT_EQ(ids.size(), 0);
}