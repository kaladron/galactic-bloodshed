#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

# Development Environment Setup Script for Galactic Bloodshed
# Installs the required versions of CMake 4.0+ and LLVM/Clang 19

set -e

echo "🚀 Setting up Galactic Bloodshed development environment..."

# Check if running on Ubuntu/Debian
if ! command -v apt-get &> /dev/null; then
    echo "❌ This script is designed for Ubuntu/Debian systems"
    echo "   For other systems, please see .devcontainer/README.md"
    exit 1
fi

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    SUDO=""
else
    SUDO="sudo"
    echo "🔐 This script requires sudo access for package installation"
fi

# Update package list
echo "📦 Updating package list..."
$SUDO apt-get update

# Install prerequisites
echo "🔧 Installing prerequisites..."
$SUDO apt-get install -y \
    curl \
    wget \
    ca-certificates \
    gnupg \
    software-properties-common \
    build-essential \
    git

# Install LLVM/Clang 19
echo "🦙 Installing LLVM/Clang 19..."
if ! command -v clang-19 &> /dev/null; then
    wget https://apt.llvm.org/llvm.sh
    chmod +x llvm.sh
    $SUDO ./llvm.sh 19 all
    rm llvm.sh
    
    # Set Clang 19 as default
    $SUDO update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100
    $SUDO update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100
    $SUDO update-alternatives --install /usr/bin/lld lld /usr/bin/lld-19 100
else
    echo "   ✅ Clang 19 already installed"
fi

# Install latest CMake from Kitware
echo "🏗️  Installing latest CMake..."
CMAKE_VERSION=$(cmake --version 2>/dev/null | head -n1 | cut -d' ' -f3 || echo "0.0.0")
if [[ $(echo "$CMAKE_VERSION 4.0.0" | tr ' ' '\n' | sort -V | head -n1) != "4.0.0" ]]; then
    # Add Kitware repository
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | $SUDO tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    $SUDO apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
    $SUDO apt-get update
    $SUDO apt-get install -y cmake
else
    echo "   ✅ CMake 4.0+ already installed"
fi

# Install project dependencies
echo "📚 Installing project dependencies..."
$SUDO apt-get install -y \
    libsqlite3-dev \
    sqlite3 \
    ninja-build \
    doxygen \
    graphviz

# Set environment variables
echo "🌍 Setting up environment variables..."
cat >> ~/.bashrc << 'EOF'

# Galactic Bloodshed development environment
export CC=/usr/bin/clang-19
export CXX=/usr/bin/clang++-19
export CMAKE_CXX_COMPILER=/usr/bin/clang++-19
export CMAKE_C_COMPILER=/usr/bin/clang-19
EOF

# Apply environment variables to current session
export CC=/usr/bin/clang-19
export CXX=/usr/bin/clang++-19
export CMAKE_CXX_COMPILER=/usr/bin/clang++-19
export CMAKE_C_COMPILER=/usr/bin/clang-19

# Verify installations
echo ""
echo "✅ Installation complete! Installed versions:"
echo "   CMake: $(cmake --version | head -n1)"
echo "   Clang: $(clang-19 --version | head -n1)"
echo "   Clang++: $(clang++-19 --version | head -n1)"
echo ""
echo "🔄 Please restart your shell or run 'source ~/.bashrc' to apply environment variables"
echo "🏗️  You can now build the project with:"
echo "   cd /path/to/galactic-bloodshed"
echo "   mkdir -p build && cd build"
echo "   cmake .."
echo "   cmake --build ."