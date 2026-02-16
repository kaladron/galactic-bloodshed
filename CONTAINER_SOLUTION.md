# CI Container Permissions - Solution Options

## Quick Answer

**YES, you should fix this in the container!** The current `--user root` approach works but isn't ideal. Here's what you need to know:

## The Problem

Your container already creates a user (`vscode`) but with **UID 1000**:
```dockerfile
ARG USER_UID=1000  # ← This is the issue
```

GitHub Actions runner needs **UID 1001**. This mismatch causes permission errors.

## Current Workaround (Temporary)
- ✅ Works now: `options: --user root` in `.github/workflows/ci.yml`
- ❌ Not ideal: Violates principle of least privilege
- ⚠️ Use temporarily until container is updated

## Solution Options (Pick One)

### Option 1: Change Default UID to 1001 (Recommended)

In your container's Dockerfile, change this line:

```dockerfile
# BEFORE:
ARG USER_UID=1000

# AFTER:
ARG USER_UID=1001
```

That's it! The rest of your Dockerfile already handles user creation properly.

### Option 2: Build with UID 1001

Keep Dockerfile as-is, but build with:
```bash
docker build --build-arg USER_UID=1001 -t ghcr.io/kaladron/cpp-image/dev-env:latest .
```

### Option 3: Use --user 1001 Instead of Root (Safer)

In `.github/workflows/ci.yml`, change:
```yaml
options: --user root  # ← Current

# To:
options: --user 1001:1001  # ← Safer (matches GitHub Actions UID)
```

This is safer than `root` but less ideal than fixing the container.

### Bonus: Move Dependencies Into Container (Recommended!)

Your CI workflow installs these every run. Move them to the Dockerfile for faster CI:

```dockerfile
# Add after LLVM installation, before USER switch:
RUN apt-get update && \
    apt-get install -y libsqlite3-dev && \
    # Create symlinks for tools
    ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    # Create C++ modules symlink
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1 && \
    # Clean up
    apt-get clean && rm -rf /var/lib/apt/lists/*
```

This eliminates the entire "Install dependencies" step from CI!

## After Container Update

Once you rebuild and push the container with UID 1001:

### Update `.github/workflows/ci.yml`:

1. **Remove** the `options: --user root` line
2. **Either:**
   - Delete the "Install dependencies" step entirely (if you baked them into container), OR
   - Add `sudo` before commands in that step (if dependencies aren't in container)

### If Using Option 3 (--user 1001)

Change from:
```yaml
options: --user root
```

To:
```yaml
options: --user 1001:1001
```

No other changes needed! The container already has the user, just wrong UID.

## Files in This Repository

- **`CONTAINER_FIX_GUIDE.md`**: Complete detailed guide with examples
- **`ci-after-container-fix.yml`**: Example workflow file to use after container is updated

## Benefits

✅ More secure (no root access needed)  
✅ Faster CI (no package installation step)  
✅ Cleaner workflow  
✅ Better matches local development environment  
✅ Works with devcontainer too  

## Need Help?

See `CONTAINER_FIX_GUIDE.md` for complete step-by-step instructions, testing procedures, and troubleshooting.
