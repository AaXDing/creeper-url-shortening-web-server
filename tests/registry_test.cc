#include "registry.h"

#include "echo_request_handler.h"
#include "gtest/gtest.h"

// Type aliases to improve readability
typedef std::function<std::unique_ptr<RequestHandler>(
    const std::string&, std::shared_ptr<RequestHandlerArgs>)>
    RequestHandlerFactory;
typedef std::function<std::shared_ptr<RequestHandlerArgs>(
    std::shared_ptr<NginxConfigStatement>)>
    CreateFromConfigFactory;
typedef std::unordered_map<std::string, RequestHandlerFactory> FactoryMap;
typedef std::unordered_map<std::string, CreateFromConfigFactory> CreateFromConfigMap;

class RegistryTest : public ::testing::Test {};

// Because echo_request_handler.cc already does:
//    REGISTER_HANDLER("EchoHandler", EchoRequestHandler);
// we can just look it up directly:

TEST_F(RegistryTest, MapSizes) {
  // Get maps
  const FactoryMap& factory_map = Registry::get_factory_map();
  const CreateFromConfigMap& create_from_config_map =
      Registry::get_create_from_config_map();

  // Maps should have at least one entry (EchoHandler)
  EXPECT_GE(factory_map.size(), 1);
  EXPECT_GE(create_from_config_map.size(), 1);

  // Maps should have the same number of entries
  EXPECT_EQ(factory_map.size(), create_from_config_map.size());
}

TEST_F(RegistryTest, BuiltInFactoryRegistration) {
  std::shared_ptr<RequestHandlerFactory> factory =
      Registry::get_handler_factory("EchoHandler");
  ASSERT_NE(factory, nullptr);
}

TEST_F(RegistryTest, BuiltInCreateFromConfigRegistration) {
  std::shared_ptr<CreateFromConfigFactory> create_from_config_fn =
      Registry::get_create_from_config("EchoHandler");
  ASSERT_NE(create_from_config_fn, nullptr);
}

TEST_F(RegistryTest, NonexistentFactory) {
  EXPECT_EQ(Registry::get_handler_factory("NoSuchHandler"), nullptr);
}

TEST_F(RegistryTest, NonexistentCreateFromConfig) {
  EXPECT_EQ(Registry::get_create_from_config("NoSuchHandler"), nullptr);
}

TEST_F(RegistryTest, SingletonMaps) {
  // Both maps are singletons
  const FactoryMap& fm1 = Registry::get_factory_map();
  const FactoryMap& fm2 = Registry::get_factory_map();
  const CreateFromConfigMap& cm1 = Registry::get_create_from_config_map();
  const CreateFromConfigMap& cm2 = Registry::get_create_from_config_map();

  EXPECT_EQ(&fm1, &fm2);
  EXPECT_EQ(&cm1, &cm2);
}
