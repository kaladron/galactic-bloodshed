# Quick Container Changes for GitHub Actions

Based on your existing Dockerfile at https://github.com/kaladron/cpp-image/blob/main/.devcontainer/Dockerfile

## Minimal Change (Recommended)

**Change 1 line:**

```dockerfile
# Line ~24 in your Dockerfile:
# BEFORE:
ARG USER_UID=1000

# AFTER:
ARG USER_UID=1001
```

That's all you need! Rebuild and push.

## Recommended Addition: Bake in Dependencies

**Add this block** after your LLVM installation (around line 100), **before** the oh-my-zsh installation:

```dockerfile
# Install project-specific dependencies for galactic-bloodshed
RUN apt-get update && \
    apt-get install -y --no-install-recommends libsqlite3-dev && \
    ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
```

## Full Section Context

Your Dockerfile around line 20-40 should look like:

```dockerfile
USER root

# Create vscode user similar to base image
ARG USERNAME=vscode
ARG USER_UID=1001  # ← Changed from 1000
ARG USER_GID=$USER_UID

RUN if id -u $USER_UID &>/dev/null; then \
        existing_user=$(id -un $USER_UID); \
        echo "Removing existing user $existing_user with UID $USER_UID"; \
        userdel -r $existing_user || true; \
    fi \
    && if getent group $USER_GID &>/dev/null; then \
        existing_group=$(getent group $USER_GID | cut -d: -f1); \
        echo "Removing existing group $existing_group with GID $USER_GID"; \
        groupdel $existing_group || true; \
    fi \
    && groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    # Add sudo support for the non-root user
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME
```

## Build and Push

```bash
cd /path/to/cpp-image
docker build -t ghcr.io/kaladron/cpp-image/dev-env:latest .
docker push ghcr.io/kaladron/cpp-image/dev-env:latest
```

## What This Fixes

- ✅ GitHub Actions can write to mounted volumes
- ✅ No need for `--user root` in workflow
- ✅ Faster CI (no apt-get on every run if you add dependencies)
- ✅ Works with both CI and devcontainer

## After Pushing Updated Container

In galactic-bloodshed repo's `.github/workflows/ci.yml`:

1. **Remove** this line:
   ```yaml
   options: --user root
   ```

2. **Delete** the "Install dependencies" step (if you added them to container)

That's it!
