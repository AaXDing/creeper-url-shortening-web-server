# The following code is modified based on LLM outputs
# Stage: coverage builder
FROM creeper:base AS coverage

# Copy the full source tree
COPY . /usr/src/projects/creeper
WORKDIR /usr/src/projects/creeper/build

# Configure for coverage
RUN cmake -DCMAKE_BUILD_TYPE=Coverage ..
# Generate the summary report
RUN make coverage   
