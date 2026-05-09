# Dockerfile — Mini Math Language
# Builds and runs MML on Ubuntu 22.04

FROM ubuntu:22.04

# Suppress interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive

# Install toolchain
RUN apt-get update && apt-get install -y \
    bison \
    flex  \
    gcc   \
    make  \
    && rm -rf /var/lib/apt/lists/*

# Copy project files into the image
WORKDIR /mml
COPY mml/ .

# Build the mml binary
RUN make

# Default: run all five sample programs
CMD ["make", "test"]
