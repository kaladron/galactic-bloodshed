# Container Fix Guide: Proper Permissions for GitHub Actions

## TL;DR - What You Need to Do

**Yes, you should fix this in the container instead of using `--user root`!** Here's what needs to change in your container image (`ghcr.io/kaladron/cpp-image/dev-env:latest`).

## The Problem

GitHub Actions mounts workspace directories with the runner user's UID (typically 1001). Your container runs as a different user by default, causing permission conflicts. The current workaround (`--user root`) works but violates the principle of least privilege.

## The Solution: Two Approaches

### Option 1: Set Default User to UID 1001 (Recommended)

Add this to your Dockerfile:

```dockerfile
# Create a user that matches GitHub Actions runner UID
RUN groupadd -g 1001 runner && \
    useradd -m -u 1001 -g runner runner

# Switch to the runner user
USER 1001
```

**Place this near the end of your Dockerfile**, after installing all system packages but before any USER-specific setup.

### Option 2: Make Directories World-Writable (Less secure, but simpler)

If you need the container to work with multiple UIDs:

```dockerfile
# Make common working directories writable by all users
RUN mkdir -p /workspace && \
    chmod 777 /workspace
```

Then set `WORKDIR /workspace` and ensure temp directories are writable.

## Detailed Implementation (Option 1 - Recommended)

Here's a complete example for your Dockerfile:

```dockerfile
# ... existing Dockerfile content ...
# (all your LLVM, CMake, dependencies, etc.)

# Near the end, before the final USER directive:

# Create runner user matching GitHub Actions UID/GID
RUN groupadd -g 1001 runner && \
    useradd -m -u 1001 -g runner -s /bin/bash runner && \
    # Give runner user sudo access if needed for dev work
    echo "runner ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/runner && \
    chmod 0440 /etc/sudoers.d/runner

# Create directories that might be needed and set ownership
RUN mkdir -p /workspace /home/runner/.cache && \
    chown -R runner:runner /home/runner

# Set the default user
USER runner

# Set working directory
WORKDIR /home/runner
```

## After Updating the Container

Once you rebuild and push the updated container:

### 1. Remove the `--user root` workaround

In `.github/workflows/ci.yml`, change:

```yaml
container:
  image: ghcr.io/kaladron/cpp-image/dev-env:latest
  credentials:
    username: ${{ github.actor }}
    password: ${{ secrets.GITHUB_TOKEN }}
  options: --user root  # ← Remove this line
```

To:

```yaml
container:
  image: ghcr.io/kaladron/cpp-image/dev-env:latest
  credentials:
    username: ${{ github.actor }}
    password: ${{ secrets.GITHUB_TOKEN }}
  # No options needed - container already runs as UID 1001
```

### 2. Update the Install Dependencies Step (Important!)

Since the container will no longer run as root by default, you'll need to use `sudo` for system package installations:

```yaml
- name: Install dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y libsqlite3-dev
    # Create symlinks (these may also need sudo depending on permissions)
    sudo ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld
    sudo ln -sf /usr/bin/lld-22 /usr/bin/lld
    sudo ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps
    sudo ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format
    sudo mkdir -p /lib/share/libc++
    sudo ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1
```

**OR BETTER**: Move these installations/symlinks into the Dockerfile itself so they're baked into the container!

## Even Better: Bake Dependencies Into Container

The ideal solution is to move the "Install dependencies" step entirely into your container build:

```dockerfile
# In your Dockerfile, add:
RUN apt-get update && \
    apt-get install -y libsqlite3-dev && \
    # Create symlinks for lld linker
    ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    # Update clang-scan-deps
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    # Create symlink for clang-format
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    # Create C++ standard library modules symlink
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1 && \
    # Clean up to reduce image size
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
```

Then you can **completely remove** the "Install dependencies" step from your CI workflow!

## Complete Dockerfile Example Structure

```dockerfile
FROM ubuntu:24.04

# Install system packages and build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    clang-22 \
    libc++-22-dev \
    libc++abi-22-dev \
    lld-22 \
    libsqlite3-dev \
    git \
    # ... other dependencies ...
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create all necessary symlinks
RUN ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1

# Create runner user (matching GitHub Actions UID)
RUN groupadd -g 1001 runner && \
    useradd -m -u 1001 -g runner -s /bin/bash runner && \
    echo "runner ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/runner && \
    chmod 0440 /etc/sudoers.d/runner

# Switch to runner user
USER runner
WORKDIR /home/runner

# Set environment variables if needed
ENV CXX=clang++-22
ENV CC=clang-22
```

## Benefits of This Approach

1. **Security**: Follows principle of least privilege - no root access needed
2. **Simplicity**: Cleaner CI workflow with fewer steps
3. **Speed**: No package installation on every CI run
4. **Consistency**: Same environment everywhere (CI, devcontainer, local)
5. **Debugging**: Easier to reproduce CI issues locally

## Migration Steps

1. **Update your container Dockerfile** with the changes above
2. **Rebuild the container**: `docker build -t ghcr.io/kaladron/cpp-image/dev-env:latest .`
3. **Push to registry**: `docker push ghcr.io/kaladron/cpp-image/dev-env:latest`
4. **Update `.github/workflows/ci.yml`**: Remove `options: --user root` and simplify/remove install step
5. **Test**: Push a commit and verify CI passes
6. **Update `.devcontainer/devcontainer.json`** if needed (should work automatically since it uses same image)

## Testing Your Container Changes Locally

Before pushing to the registry:

```bash
# Build the container
docker build -t test-container .

# Test that it runs as UID 1001
docker run --rm test-container id
# Should output: uid=1001(runner) gid=1001(runner) groups=1001(runner)

# Test file permissions work
docker run --rm -v $(pwd):/workspace test-container bash -c "cd /workspace && touch test.txt && rm test.txt"
# Should succeed without permission errors

# Test building your project
docker run --rm -v $(pwd):/workspace test-container bash -c "cd /workspace && cmake -B build && cmake --build build"
```

## Questions or Issues?

If you run into any issues updating the container, let me know and I can help troubleshoot!
