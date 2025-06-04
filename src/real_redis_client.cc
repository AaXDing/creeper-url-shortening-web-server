#include "real_redis_client.h"

using namespace sw::redis;

RealRedisClient::RealRedisClient(const std::string& redis_ip, int redis_port)
    : pool_(std::make_shared<RedisConnectionPool>(redis_ip, redis_port)) {
    // The connection pool constructor will verify connections are working
}

std::optional<std::string> RealRedisClient::get(const std::string& short_code) {
    try {
        auto conn = pool_->acquire();
        auto result = conn->get(short_code);
        pool_->release(conn);
        return result;
    } catch (const Error& e) {
        LOG(error) << "Redis GET error for key \"" << short_code
                   << "\": " << e.what();
        return std::nullopt;
    }
}

void RealRedisClient::set(const std::string& short_code,
                          const std::string& long_url) {
    try {
        auto conn = pool_->acquire();
        conn->set(short_code, long_url);
        pool_->release(conn);
    } catch (const Error& e) {
        LOG(fatal) << "Redis SET error for key \"" << short_code << "\" => \""
                   << long_url << "\": " << e.what();
        exit(1);
    }
}