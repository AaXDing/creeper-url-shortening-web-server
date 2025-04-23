# The following code is modified based on LLM outputs
# Stage: coverage builder
FROM creeper:base AS coverage

# Copy the full source tree
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Configure for coverage
RUN cmake -DCMAKE_BUILD_TYPE=Coverage ..
# Generate the summary report
RUN make coverage   
