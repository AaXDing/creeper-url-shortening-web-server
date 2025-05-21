# Usage: docker build -t creeper-server -f docker/Dockerfile .
#        docker run -p 127.0.0.1:8080:8080 creeper-server
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
RUN ctest --output-on-failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble AS deploy

# Make directory for app and static
RUN mkdir -p /app
RUN mkdir -p /data
# Copy server output binary to "/app"
COPY --from=builder /usr/src/projects/creeper/build/bin/server /app/
# Copy config file as well
COPY --from=builder /usr/src/projects/creeper/my_config /app/
# Copy static files to "."
COPY --from=builder /usr/src/projects/creeper/data /data
# Set working directory
WORKDIR /app

# Expose some port(s)
# EXPOSE 8080
EXPOSE 80

# Use ENTRYPOINT to specify the binary name
# Update with real server name
ENTRYPOINT ["./server"]

# Use CMD to specify arguments to ENTRYPOINT
# Update with real server args
# CMD ["8080"]
# Pass in the config file 
CMD ["my_config"]
