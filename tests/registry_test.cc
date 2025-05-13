#include "registry.h"

#include "echo_request_handler.h"
#include "gtest/gtest.h"

// Type aliases to improve readability
typedef std::function<std::unique_ptr<RequestHandler>(const std::string&,
                                                      const std::string&)>
    HandlerFactory;
typedef std::function<bool(std::shared_ptr<NginxConfigStatement>,
                           NginxLocation&)>
    LocationCheckFn;
typedef std::unordered_map<std::string, HandlerFactory> FactoryMap;
typedef std::unordered_map<std::string, LocationCheckFn> CheckLocationMap;

class RegistryTest : public ::testing::Test {};

// Because echo_request_handler.cc already does:
//    REGISTER_HANDLER("EchoHandler", EchoRequestHandler);
// we can just look it up directly:

TEST_F(RegistryTest, MapSizes) {
  // Get maps
  const FactoryMap& factory_map = Registry::get_factory_map();
  const CheckLocationMap& check_location_map =
      Registry::get_check_location_map();

  // Maps should have at least one entry (EchoHandler)
  EXPECT_GE(factory_map.size(), 1);
  EXPECT_GE(check_location_map.size(), 1);

  // Maps should have the same number of entries
  EXPECT_EQ(factory_map.size(), check_location_map.size());
}

TEST_F(RegistryTest, BuiltInFactoryRegistration) {
  std::shared_ptr<HandlerFactory> factory =
      Registry::get_handler_factory("EchoHandler");
  ASSERT_NE(factory, nullptr);
}

TEST_F(RegistryTest, BuiltInCheckLocationRegistration) {
  std::shared_ptr<LocationCheckFn> check_fn =
      Registry::get_check_location("EchoHandler");
  ASSERT_NE(check_fn, nullptr);
}

TEST_F(RegistryTest, NonexistentFactory) {
  EXPECT_EQ(Registry::get_handler_factory("NoSuchHandler"), nullptr);
}

TEST_F(RegistryTest, NonexistentCheckLocation) {
  EXPECT_EQ(Registry::get_check_location("NoSuchHandler"), nullptr);
}

TEST_F(RegistryTest, SingletonMaps) {
  // Both maps are singletons
  const FactoryMap& fm1 = Registry::get_factory_map();
  const FactoryMap& fm2 = Registry::get_factory_map();
  const CheckLocationMap& cm1 = Registry::get_check_location_map();
  const CheckLocationMap& cm2 = Registry::get_check_location_map();

  EXPECT_EQ(&fm1, &fm2);
  EXPECT_EQ(&cm1, &cm2);
}
