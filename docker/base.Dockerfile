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