# Development Container for Galactic Bloodshed

This directory contains the development container configuration for Galactic Bloodshed, providing a consistent development environment with the exact tool versions required for this C++26 project.

## 🎯 Purpose

The Galactic Bloodshed project requires:
- **CMake 4.0+** (for C++ modules support)
- **LLVM/Clang 19** with libc++ (for C++26 features)
- **Ninja** build system
- **SQLite3** development libraries

Many standard development environments have older versions of these tools, causing build failures. This custom container solves that problem.

## 🚀 Quick Start

### For GitHub Copilot Agent (Recommended)

Edit `devcontainer.json` to use the pre-built image:

```json
{
  "name": "Galactic Bloodshed C++26 Development",
  "image": "ghcr.io/kaladron/galactic-bloodshed/devcontainer:latest",
  // ... rest of configuration
}
```

This provides the fastest startup time and is automatically updated via GitHub Actions.

### For Local Development

Use the build configuration (default):

```json
{
  "name": "Galactic Bloodshed C++26 Development", 
  "build": {
    "dockerfile": "Dockerfile",
    "context": "."
  },
  // ... rest of configuration
}
```

This builds the image locally using the current Dockerfile.

## 📦 What's Included

The development container includes:

- **Ubuntu 24.04** base system
- **CMake 4.x** (latest from Kitware)
- **LLVM/Clang 19** with full toolchain
- **libc++** standard library
- **Ninja** build system  
- **SQLite3** with development headers
- **Development tools**: git, build-essential, python3, etc.
- **Documentation tools**: doxygen, graphviz

## 🔧 Environment Variables

The container sets appropriate defaults:

```bash
CC=/usr/bin/clang-19
CXX=/usr/bin/clang++-19
CMAKE_CXX_COMPILER=/usr/bin/clang++-19
CMAKE_C_COMPILER=/usr/bin/clang-19
```

## 🏗️ Building Locally

To build the image manually:

```bash
cd .devcontainer
docker build -t galactic-bloodshed-dev .
```

## 🔄 Automated Builds

The container image is automatically built and published via GitHub Actions when:
- Changes are made to `.devcontainer/` files
- Changes are made to `CMakeLists.txt` or `cmake/` files  
- Manually triggered via workflow dispatch

## 🐛 Troubleshooting

**Image build fails**: Check the GitHub Actions log for the latest build status.

**CMake version issues**: Verify you're using the custom container - standard images often have CMake 3.x.

**Clang module errors**: Ensure you're using the container's Clang 19, not the system compiler.

**Permission issues**: The container creates a non-root user `devuser` for development.

## 📚 Related Documentation

- [AGENTS.md](../AGENTS.md) - Full development guide
- [docs/COPILOT_TASKS.md](../docs/COPILOT_TASKS.md) - Task-specific guidance