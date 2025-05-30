# Creeper Server

A simple asynchronous TCP server built with Boost.Asio.

## Project Structure

The project follows a modular architecture with clear separation of concerns:

```
creeper/
├── include/           # Header files
├── src/               # Source files
├── tests/             # Test files
│   ├── dispatcher_testcases/       # Request handler dispatcher test configs
│   ├── integration_testcases/      # Integration test configs
│   ├── config_testcases/           # Configuration test confis
│   └── request_handler_tesetcases  # Request handler test configs
├── data/              # Static files for testing
└── cmake/             # CMake configuration files
```

### Key Components

1. **Server (`server.h/cc`)**: Main server implementation using Boost.Asio
2. **Session (`session.h/cc`)**: Handles individual client connections
3. **Request Parser (`request_parser.h/cc`)**: Parses incoming HTTP requests
4. **Request Handlers**:
   - `echo_request_handler.h/cc`: Echoes back the request
   - `static_request_handler.h/cc`: Serves static files
   - `not_found_request_handler.h/cc`: Returns 404 Not Found response when no other handler can handle a request (Need to setup config file correctly)
   - `crud_request_handler.h/cc`: Handles CRUD operations (Create, Retrieve, Update, Delete) for data persistence
   - `health_request_handler.h/cc`: Returns 200 OK response to indicate server is healthy
   - `blocking_request_handler.h/cc`: Blocks the request for 3 seconds and return 200 OK
   - `shorten_request_hanlder.h/cc`: Shortens URL and resolve URL
5. **Request Handler Dispatcher (`request_handler_dispatcher.h/cc`)**: Routes requests to appropriate handlers
6. **Configuration Parser (`config_parser.h/cc`)**: Parses server configuration
7. **Registry (`registry.h/cc`)**: Manages request handler registration

### Source Code Layout
<pre>
            server_main.cc  <-- Entry point
                  |
        +---------+---------+
        |                   |
 config_parser.cc       server.cc  <-- Accepts connections
                             |
              +--------------+--------------+
              |                             |
   request_handler_dispatcher.cc     session.cc  <-- Manages client session
      (owned,build routes)                  |   ----------------- > request_parser.cc (owns)
                                            | (uses)              
                                            v
                                 request_handler_dispatcher.cc <-- Build routes from config
                                             | (creates)
                                             |
                                 request_handler.h  <-- Interface
                                             |
                                echo_request_handler.cc      (impl) 
                                static_request_handler.cc    (impl)
                                not_found_request_handler.cc (impl)
                                crud_request_handler.cc      (impl)
                                health_request_handler.cc    (impl)
                                blocking_request_handler.cc  (impl)
                                shorten_request_handler.cc   (impl)

Utility Modules:
----------------
  http_header.cc   logging.cc

Factory & Registration:
------------------------
  registry.cc  <-- Registers handlers and get_location functions
</pre>

## Building and Running

### Prerequisites

The project requires the following dependencies besides CS130 Dev environment:
- Boost JSON library (libboost-json-dev)
  ```bash
  # Install Boost JSON library
  sudo apt-get install libboost-json-dev
  ```

- Redis Server (redis-server, hiredis)
 ```bash
 sudo apt-get install redis-server libhiredis-dev
 git clone https://github.com/sewenew/redis-plus-plus.git
 cd redis-plus-plus
 mkdir build
 cd build
 cmake ..
 make
 make install
 cd ../..
 rm -rf redis-plus-plus
 ```

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

3. Start Redis
```bash
redis-server ../redis.conf
```

4. Testing:
```bash
ctest
```


### Coverage Build

```bash
make coverage
```

### Build & Run

1. Build:

```bash
make build
```

2. Run Redis:
```bash
redis-server ../redis.conf --daemonize yes
```

3. Run Server:

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

### Code Formatting

The project uses clang-format for consistent code formatting. To use it:

1. Install clang-format:
```bash
sudo apt-get install clang-format
```

2. Format all source files:
```bash
make format .clang-format
```

The project uses the following clang-format style:
- Based on Google style
- 2 space indentation
- 100 character line length
- Spaces for indentation

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

#### CRUD operations
To create a new entity:
```bash
curl -X POST localhost:80/api/Name \
     -H "Content-Type: application/json" \
     -d '{"name": "Joe Bruin"}'
```

#### Shorten URL
To create a new short URL:
```bash
curl -X POST localhost:80/shorten \
     -H "Content-Type: text/plain" \
     -d 'https://code.cs130.org'
# must be a full url
curl -X GET localhost:80/shorten/fjnuNs
# will get https://code.cs130.org
```

### Docker Build
To handle static files, you need to move all your static files to `data` directory before building the docker image.
```bash
docker build -t creeper:base -f docker/base.Dockerfile .
docker build -f docker/coverage.Dockerfile -t creeper-coverage .
```
or
```bash
make docker
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
    NOT_FOUND_REQUEST_HANDLER,
    NEW_REQUEST_HANDLER,  // Add your handler type
};

// Add your handler type to the handler_type_to_string function
static std::string handler_type_to_string(HandlerType type) {
    switch (type) {
        case HandlerType::ECHO_REQUEST_HANDLER:
            return "EchoHandler";
        case HandlerType::STATIC_REQUEST_HANDLER:
            return "StaticHandler";
        case HandlerType::NOT_FOUND_REQUEST_HANDLER:
            return "NotFoundHandler";
        case HandlerType::NEW_REQUEST_HANDLER:  // Add your handler type
            return "NewHandler";
        default:
            return "UnknownHandler";
    }
}
```

4. Update CMakeLists.txt:
- Create a new library target for your request handler
`add_library(new_request_handler_lib src/new_request_handler.cc)`
- Link against required dependencies
`target_link_libraries(new_request_handler_lib PUBLIC http_header_lib logging_lib registry_lib)`

- Add your handler's source file to these executables to ensure it's included in the build:
    - server: Main server executable
    - config_parser_lib_test: Config Parser teseting
    - session_lib_test: Session testing
    - request_handler_dispatcher_lib_test: Request handler dispatcher testing

    Example: `add_executable(server ... src/new_request_handler.cc)`

- Add your handler's library target to these executables to ensure it's included in the build:
    - config_parser_lib_test: Config Parser teseting

    Example: `target_link_libraries(config_parser_lib_test ... new_request_handler_lib)`

- Add your library to the coverage report targets section

    Example: add to the TARGETS list in generate_coverage_report()

5. Create test file in `tests/`:
```cpp
#include "new_request_handler.h"
#include "gtest/gtest.h"

class NewRequestHandlerTestFixture : public ::testing::Test {
protected:
    // Set up test fixtures
};

TEST_F(NewRequestHandlerTestFixture, BasicFunctionality) {
    // Add test cases
}
```
Update `CMakeLists.txt`
- Create test executable for your request handler
`add_executable(new_request_handler_lib_test tests/new_request_handler_test.cc)`
- Link against required test dependencies
`target_link_libraries(new_request_handler_lib_test http_header_lib new_request_handler_lib config_parser_lib registry_lib logging_lib gtest_main)`

- Register the test with Google Test
`gtest_discover_tests(new_request_handler_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)`

- Add your test to the coverage report tests section
    
    Example: add to the TESTS list in generate_coverage_report()

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

location / NotFoundHandler {
}

# no trailing slash after path
location /echo EchoHandler {
}

location /static StaticHandler {
    # no trailing slash after path
    # relative-path and absolute path supported
    root ../data;
}

location /health HealthHandler {
}

location /sleep BlockingHandler{
}
```
