#include "real_redis_client.h"

using namespace sw::redis;

RealRedisClient::RealRedisClient(const std::string& redis_ip, int redis_port)
    : redis_("tcp://" + redis_ip + ":" + std::to_string(redis_port)) {
  try {
    // Force a ping to verify connection
    redis_.ping();
  } catch (const Error& e) {
    LOG(fatal) << "Failed to connect to Redis at "
               << "tcp://" << redis_ip << ":" << redis_port << ": " << e.what();
    exit(1);
  }
}

std::optional<std::string> RealRedisClient::get(const std::string& short_code) {
  try {
    return redis_.get(short_code);
  } catch (const Error& e) {
    LOG(error) << "Redis GET error for key \"" << short_code
               << "\": " << e.what();
    return std::nullopt;
  }
}

void RealRedisClient::set(const std::string& short_code,
                          const std::string& long_url) {
  try {
    redis_.set(short_code, long_url);
  } catch (const Error& e) {
    LOG(fatal) << "Redis SET error for key \"" << short_code << "\" => \""
               << long_url << "\": " << e.what();
    exit(1);
  }
}
