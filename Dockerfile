# Use a base image with a C++ environment
FROM ubuntu:latest

# Set non-interactive mode to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    libboost-all-dev \
    libglfw3 \
    libglfw3-dev \
    libglew-dev \
    libglm-dev \
    libopencv-dev \
    git

# Copy your project files to the Docker image
COPY . /NeuroCorrelation

# Set the working directory
WORKDIR /NeuroCorrelation

# Build your project
RUN mkdir build && cd build && \
    cmake .. && \
    make

# Command to run your application
CMD ["./build/NeuroCorrelation"]

