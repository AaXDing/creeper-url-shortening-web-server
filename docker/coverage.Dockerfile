# The following code is modified based on LLM outputs
# Stage: coverage builder
FROM creeper:base as coverage

# Copy the full source tree
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Configure for coverage and build + run tests
RUN cmake -DCMAKE_BUILD_TYPE=Coverage ..
RUN make
# Run tests for coverage 
RUN ctest --output-on-failure
# Generate the summary report
RUN make coverage   
