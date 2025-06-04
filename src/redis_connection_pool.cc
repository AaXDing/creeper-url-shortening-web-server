#include "redis_connection_pool.h"
#include "logging.h"

using namespace sw::redis;

RedisConnectionPool::RedisConnectionPool(const std::string& redis_ip, int redis_port, size_t pool_size)
    : connection_string_("tcp://" + redis_ip + ":" + std::to_string(redis_port))
    , pool_size_(pool_size) {
    // Initialize the connection pool

    for (size_t i = 0; i < pool_size_; ++i) {
        try {
            auto conn = std::make_shared<Redis>(connection_string_);
            // Test the connection
            conn->ping();
            connections_.push(conn);
        } catch (const Error& e) {
            LOG(fatal) << "Failed to create Redis connection: " << e.what();
            exit(1);
        }
    }
}

RedisConnectionPool::~RedisConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        connections_.pop();
    }
}

std::shared_ptr<Redis> RedisConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !connections_.empty(); });
    
    auto conn = connections_.front();
    connections_.pop();
    return conn;
}

void RedisConnectionPool::release(std::shared_ptr<Redis> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push(conn);
    cv_.notify_one();
}