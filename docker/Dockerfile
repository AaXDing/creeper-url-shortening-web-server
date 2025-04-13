# Usage: docker build -t creeper-server -f docker/Dockerfile .
#        docker run -p 127.0.0.1:8080:8080 creeper-server
#
### Build/test container ###
# Define builder stage
FROM creeper:base as builder

# Share work directory
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Build and test
RUN cmake ..
RUN make
RUN ctest --output-on_failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble as deploy

# Copy server output binary to "."
COPY --from=builder /usr/src/project/build/bin/server .
# Copy config file as well
COPY --from=builder /usr/src/project/my_config .

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
