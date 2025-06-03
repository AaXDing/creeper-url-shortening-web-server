### Base environment container ###
# Get the base Ubuntu image from Docker Hub
FROM ubuntu:noble AS base

ARG DEBIAN_FRONTEND=noninteractive

# Update the base image and install build environment
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    cmake \
    curl \
    httpie \
    libboost-log-dev \
    libboost-regex-dev \
    libboost-system-dev \
    libboost-json-dev \
    libgmock-dev \
    libgtest-dev \
    libhiredis-dev \
    libpq-dev \
    netcat-openbsd \
    redis-server \
    gcovr && \
    git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf redis-plus-plus

# Set environment variables for database and Redis configuration
ENV REDIS_IP=127.0.0.1
ENV REDIS_PORT=6379
ENV DB_HOST=10.90.80.3
ENV DB_NAME=url-mapping
ENV DB_USER=creeper-server
ENV DB_PASSWORD=creeper