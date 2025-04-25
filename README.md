# Creeper Server

A simple asynchronous TCP server built with Boost.Asio.

## Coverage Build

```bash
mkdir build_coverage
cd build_coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```

## Normal Build & Run

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run

```bash
bin/server ../my_config
```

### Sample Client Request

```bash
printf "GET / HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n" | nc localhost 80
```

## Docker Build

```bash
docker build -t creeper:base -f docker/base.Dockerfile .
docker build -f docker/coverage.Dockerfile -t creeper-coverage .
```

## GCloud Manual Submit

```bash
gcloud builds submit --config docker/cloudbuild.yaml .
```
