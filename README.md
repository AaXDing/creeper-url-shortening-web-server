# Creeper Server

A simple asynchronous TCP server built with Boost.Asio.

## Project Structure

The project follows a modular architecture with clear separation of concerns:

```
creeper/
├── include/           # Header files
├── src/              # Source files
├── tests/            # Test files
│   ├── dispatcher_testcases/    # Test cases for request handler dispatcher
│   ├── integration_testcases/   # Integration test cases
│   └── config_testcases/        # Configuration test cases
├── data/             # Static files for testing
└── cmake/            # CMake configuration files
```

### Key Components

1. **Server (`server.h/cc`)**: Main server implementation using Boost.Asio
2. **Session (`session.h/cc`)**: Handles individual client connections
3. **Request Parser (`request_parser.h/cc`)**: Parses incoming HTTP requests
4. **Request Handlers**:
   - `echo_request_handler.h/cc`: Echoes back the request
   - `static_request_handler.h/cc`: Serves static files
5. **Request Handler Dispatcher (`request_handler_dispatcher.h/cc`)**: Routes requests to appropriate handlers
6. **Configuration Parser (`config_parser.h/cc`)**: Parses server configuration
7. **Registry (`registry.h/cc`)**: Manages request handler registration

## Building and Running

### Build & Test

1. Create a build directory:
```bash
mkdir build
cd build
```

2. Configure and build:
```bash
cmake ..
make
```

3. Testing:
```bash
ctest
```


### Coverage Build

```bash
mkdir build_coverage
cd build_coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```

### Build & Run

1. Build:

```bash
mkdir build
cd build
cmake ..
make
```

2. Run:

The server can be run in different logging modes:

| Mode | Log Level | Output |
|------|-----------|---------|
| Normal | INFO+ | File |
| Trace | TRACE+ | File |
| Debug | DEBUG+ | File |

All modes output ERROR+ logs to console.

```bash
# Normal run
bin/server ../my_config

# Trace run
CREEPER_LOG_DEBUG=trace bin/server ../my_config

# Debug run
CREEPER_LOG_DEBUG=debug bin/server ../my_config
```

### Sample Client Request

#### Echo request to server
```bash
printf "GET /echo HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n" | nc localhost 80
```

#### Static file request to server

To test static file request, you need to create a `data` directory and put your static files in it.
```bash
printf "GET /static/test.html HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n" | nc localhost 80
```

### Docker Build
To handle static files, you need to move all your static files to `data` directory before building the docker image.
```bash
docker build -t creeper:base -f docker/base.Dockerfile .
docker build -f docker/coverage.Dockerfile -t creeper-coverage .
```

### GCloud Manual Submit

```bash
gcloud builds submit --config docker/cloudbuild.yaml .
```

### GCloud SSH Docker 
```bash
docker ps
docker exec -it <container_id> /bin/bash
```

## Adding a New Request Handler

To add a new request handler, follow these steps:

1. Create header file in `include/`:
```cpp
#ifndef NEW_REQUEST_HANDLER_H
#define NEW_REQUEST_HANDLER_H

#include "request_handler.h"

class NewRequestHandler : public RequestHandler {
public:
    NewRequestHandler(const std::string& arg1, const std::string& arg2);
    std::unique_ptr<Response> handle_request(const Request& req) override;
    static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                             NginxLocation& location);
    RequestHandler::HandlerType get_type() const override;
private:
    // Add private members
};

#endif
```

2. Create implementation file in `src/`:
```cpp
#include "new_request_handler.h"
#include "registry.h"

REGISTER_HANDLER("NewHandler", NewRequestHandler);

NewRequestHandler::NewRequestHandler(const std::string& arg1, const std::string& arg2) {
    // Initialize handler
}

std::unique_ptr<Response> NewRequestHandler::handle_request(const Request& req) {
    auto res = std::make_unique<Response>();
    // Implement request handling logic
    return res;
}

bool NewRequestHandler::check_location(std::shared_ptr<NginxConfigStatement> statement,
                                     NginxLocation& location) {
    // Validate configuration
    return true;
}

RequestHandler::HandlerType NewRequestHandler::get_type() const {
    return RequestHandler::HandlerType::NEW_REQUEST_HANDLER;
}
```

3. Add handler type to `request_handler.h`:
```cpp
enum class HandlerType {
    ECHO_REQUEST_HANDLER,
    STATIC_REQUEST_HANDLER,
    NEW_REQUEST_HANDLER,  // Add your handler type
};
```

4. Update CMakeLists.txt:
```cmake
add_library(new_request_handler_lib src/new_request_handler.cc)
target_link_libraries(new_request_handler_lib PUBLIC http_header_lib logging_lib registry_lib)
```

5. Create test file in `tests/`:
```cpp
#include "new_request_handler.h"
#include "gtest/gtest.h"

class NewRequestHandlerTest : public ::testing::Test {
protected:
    // Set up test fixtures
};

TEST_F(NewRequestHandlerTest, BasicFunctionality) {
    // Add test cases
}
```

### Example: Echo Request Handler

The Echo Request Handler demonstrates a simple handler implementation:

```cpp
// echo_request_handler.h
class EchoRequestHandler : public RequestHandler {
public:
    EchoRequestHandler(const std::string& arg1, const std::string& arg2);
    std::unique_ptr<Response> handle_request(const Request& req) override;
    static bool check_location(std::shared_ptr<NginxConfigStatement> statement,
                             NginxLocation& location);
    RequestHandler::HandlerType get_type() const override;
};

// echo_request_handler.cc
std::unique_ptr<Response> EchoRequestHandler::handle_request(const Request& req) {
    auto res = std::make_unique<Response>();
    if (req.valid) {
        res->status_code = 200;
        res->status_message = "OK";
        res->version = req.version;
        res->content_type = "text/plain";
        res->body = req.to_string();
    } else {
        *res = STOCK_RESPONSE.at(400);
    }
    return res;
}
```

## Configuration

The server uses a configuration file in nginx-like format:

```nginx
port 80;

# no trailing slash after path
location /echo EchoHandler {
}

location /static StaticHandler {
    # no trailing slash after path
    # relative-path and absolute path supported
    root ../data;
}
```
