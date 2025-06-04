#include "database_connection_pool.h"
#include "logging.h"

PostgresConnectionPool::PostgresConnectionPool(const std::string& db_host, const std::string& db_name,
                                             const std::string& db_user, const std::string& db_password,
                                             size_t pool_size)
    : connection_string_("host=" + db_host + " dbname=" + db_name +
                        " user=" + db_user + " password=" + db_password)
    , pool_size_(pool_size) {
    // Initialize the connection pool

    LOG(info) << "Creating PostgreSQL connection pool with " << pool_size_ << " connections";
    
    for (size_t i = 0; i < pool_size_; ++i) {
        PGconn* conn = PQconnectdb(connection_string_.c_str());
        if (PQstatus(conn) != CONNECTION_OK) {
            std::string err = PQerrorMessage(conn);
            PQfinish(conn);
            LOG(fatal) << "Failed to create PostgreSQL connection: " << err;
            exit(1);
        }
        connections_.push(conn);
    }

    LOG(info) << "PostgreSQL connection pool created with " << pool_size_ << " connections";
}

PostgresConnectionPool::~PostgresConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        PQfinish(connections_.front());
        connections_.pop();
    }
}

PGconn* PostgresConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !connections_.empty(); });
    
    PGconn* conn = connections_.front();
    connections_.pop();
    return conn;
}

void PostgresConnectionPool::release(PGconn* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push(conn);
    cv_.notify_one();
} 