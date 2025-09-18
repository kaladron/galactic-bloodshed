# SPDX-License-Identifier: Apache-2.0

# Custom development environment for Galactic Bloodshed with latest CMake and Clang
FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential packages and dependencies
RUN apt-get update && apt-get install -y \
    # Essential build tools
    build-essential \
    git \
    curl \
    wget \
    ca-certificates \
    gnupg \
    software-properties-common \
    # Python for CMake and other tools
    python3 \
    python3-pip \
    # SQLite3 development libraries (project requirement)
    libsqlite3-dev \
    sqlite3 \
    # Documentation tools mentioned in current config
    doxygen \
    graphviz \
    # Ninja build system
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Install latest LLVM/Clang (version 19)
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 19 all && \
    rm llvm.sh

# Set Clang 19 as default
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100 && \
    update-alternatives --install /usr/bin/lld lld /usr/bin/lld-19 100

# Install latest CMake (4.x series)
# Using Kitware's official APT repository for the latest version
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" && \
    apt-get update && \
    apt-get install -y cmake && \
    rm -rf /var/lib/apt/lists/*

# Verify installations
RUN echo "=== Installed Versions ===" && \
    clang --version && \
    clang++ --version && \
    cmake --version && \
    ninja --version

# Set environment variables for C++ development
ENV CC=/usr/bin/clang-19
ENV CXX=/usr/bin/clang++-19
ENV CMAKE_CXX_COMPILER=/usr/bin/clang++-19
ENV CMAKE_C_COMPILER=/usr/bin/clang-19

# Create a non-root user for development
ARG USERNAME=devuser
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME && \
    useradd --uid $USER_UID --gid $USER_GID -m $USERNAME && \
    apt-get update && \
    apt-get install -y sudo && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME && \
    rm -rf /var/lib/apt/lists/*

# Switch to non-root user
USER $USERNAME

# Set working directory
WORKDIR /workspaces/galactic-bloodshed

# Default command
CMD ["/bin/bash"]