#include "real_database_client.h"

RealDatabaseClient::RealDatabaseClient(const std::string& db_host,
                                       const std::string& db_name,
                                       const std::string& db_user,
                                       const std::string& db_password,
                                       int pool_size)   
    : pool_(std::make_shared<PostgresConnectionPool>(db_host, db_name, db_user, db_password, pool_size)) {
    // The connection pool constructor will verify connections are working
}

bool RealDatabaseClient::store(const std::string& short_code,
                               const std::string& long_url) {
    auto conn = pool_->acquire();
    
    const int nParams = 2;
    const char* paramValues[nParams] = {short_code.c_str(), long_url.c_str()};
    const int paramLengths[nParams] = {static_cast<int>(short_code.size()),
                                     static_cast<int>(long_url.size())};
    const int paramFormats[nParams] = {0, 0};  // text format

    const char* query =
        "INSERT INTO short_to_long_url (short_url, long_url) "
        "VALUES ($1, $2) "
        "ON CONFLICT (short_url) DO UPDATE SET long_url = EXCLUDED.long_url";

    // Send query asynchronously
    int sendStatus = PQsendQueryParams(conn, query, nParams,
                                     nullptr,  // let libpq infer parameter types
                                     paramValues, paramLengths, paramFormats,
                                     0  // request text results
    );

    if (sendStatus != 1) {
        std::string err = PQerrorMessage(conn);
        LOG(fatal) << "Postgres STORE error (key=" << short_code << "): " << err;
        pool_->release(conn);
        return false;
    }

    // Wait for and process the result
    bool success = false;
    while (true) {
        // Consume any available input
        if (PQconsumeInput(conn) == 0) {
            std::string err = PQerrorMessage(conn);
            LOG(fatal) << "Postgres STORE error (key=" << short_code << "): " << err;
            break;
        }

        // Check if we need to wait for more data
        if (PQisBusy(conn)) {
            // In a real application, you would use select/epoll here
            // For simplicity, we'll just continue the loop
            continue;
        }

        // Get the result
        PGresult* res = PQgetResult(conn);
        if (res == nullptr) {
            // No more results
            break;
        }

        if (PQresultStatus(res) == PGRES_COMMAND_OK) {
            success = true;
        } else {
            std::string err = PQerrorMessage(conn);
            LOG(fatal) << "Postgres STORE error (key=" << short_code << "): " << err;
        }

        PQclear(res);
    }

    pool_->release(conn);
    return success;
}

std::optional<std::string> RealDatabaseClient::lookup(
    const std::string& short_code) {
    auto conn = pool_->acquire();
    
    const int nParams = 1;
    const char* paramValues[nParams] = {short_code.c_str()};
    const int paramLengths[nParams] = {static_cast<int>(short_code.size())};
    const int paramFormats[nParams] = {0};  // text format

    const char* query =
        "SELECT long_url FROM short_to_long_url WHERE short_url = $1";

    // Send query asynchronously
    int sendStatus = PQsendQueryParams(conn, query, nParams, nullptr, paramValues,
                                     paramLengths, paramFormats, 0);
    
    if (sendStatus != 1) {
        std::string err = PQerrorMessage(conn);
        LOG(fatal) << "Postgres LOOKUP error (key=" << short_code << "): " << err;
        pool_->release(conn);
        return std::nullopt;
    }

    // Wait for and process the result
    std::optional<std::string> result;
    while (true) {
        // Consume any available input
        if (PQconsumeInput(conn) == 0) {
            std::string err = PQerrorMessage(conn);
            LOG(fatal) << "Postgres LOOKUP error (key=" << short_code << "): " << err;
            break;
        }

        // Check if we need to wait for more data
        if (PQisBusy(conn)) {
            // In a real application, you would use select/epoll here
            // For simplicity, we'll just continue the loop
            continue;
        }

        // Get the result
        PGresult* res = PQgetResult(conn);
        if (res == nullptr) {
            // No more results
            break;
        }

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string err = PQerrorMessage(conn);
            LOG(fatal) << "Postgres LOOKUP error (key=" << short_code << "): " << err;
            PQclear(res);
            break;
        }

        if (PQntuples(res) > 0) {
            result = PQgetvalue(res, 0, 0);
        }

        PQclear(res);
    }

    pool_->release(conn);
    return result;
}
