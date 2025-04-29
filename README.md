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

Echo request to server
```bash
printf "GET /echo HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n" | nc localhost 80
```

Static file request to server
To test static file request, you need to create a `data` directory and put your static files in it.
```bash
printf "GET /static/test.html HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n" | nc localhost 80
```

## Docker Build
To handle static files, you need to move all your static files to `data` directory before building the docker image.
```bash
docker build -t creeper:base -f docker/base.Dockerfile .
docker build -f docker/coverage.Dockerfile -t creeper-coverage .
```

## GCloud Manual Submit

```bash
gcloud builds submit --config docker/cloudbuild.yaml .
```

## GCloud SSH Docker 
```bash
docker ps
docker exec -it <container_id> /bin/bash
```
