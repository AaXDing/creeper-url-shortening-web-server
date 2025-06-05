# Usage: docker build -t creeper-server -f docker/Dockerfile .
#        docker run -p 127.0.0.1:80:80 creeper-server
#
### Build/test container ###
# Define builder stage
FROM creeper:base AS builder

# Share work directory
COPY . /usr/src/projects/creeper
WORKDIR /usr/src/projects/creeper/build

RUN apt-get update

# Build and test
RUN cmake ..
RUN make
RUN redis-server ../redis.conf --daemonize yes && ctest --output-on-failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble AS deploy

# Install Redis
RUN apt-get update && \
    apt-get install -y redis-server libhiredis-dev git build-essential cmake postgresql-client && \
    git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf redis-plus-plus && \
    apt-get remove -y git build-essential cmake && \
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Make directory for app and static
RUN mkdir -p /app
RUN mkdir -p /data
# Create storage directory for CRUD operations
RUN mkdir -p /mnt/storage/crud && \
    chmod 755 /mnt/storage/crud

# Copy server output binary to "/app"
COPY --from=builder /usr/src/projects/creeper/build/bin/server /app/
# Copy config file as well
COPY --from=builder /usr/src/projects/creeper/server_config /app/
COPY --from=builder /usr/src/projects/creeper/redis.conf /app/
# Copy static files to "."
COPY --from=builder /usr/src/projects/creeper/data /data
# Set working directory
WORKDIR /app

# Create startup script
RUN echo '#!/bin/bash\nredis-server redis.conf --daemonize yes\n./server "$@"' > start.sh && \
    chmod +x start.sh

# Expose some port(s)
EXPOSE 80

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./start.sh"]

# Use CMD to specify arguments to ENTRYPOINT
CMD ["server_config"]
