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

# TODO(!): Copy server output binary to "."
COPY --from=builder /usr/src/project/build/bin/server .

# TODO(!): Expose some port(s)
EXPOSE 8080

# Use ENTRYPOINT to specify the binary name
# TODO(!): Update with real server name
ENTRYPOINT ["./server"]

# Use CMD to specify arguments to ENTRYPOINT
# TODO(!): Update with real server args
CMD ["8080"]
