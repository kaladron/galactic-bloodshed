# CI Container Permissions - Solution Options

## Quick Answer

**YES, you should fix this in the container!** The current `--user root` approach works but isn't ideal. Here's what you need to know:

## Current Workaround (Temporary)
- ✅ Works now: `options: --user root` in `.github/workflows/ci.yml`
- ❌ Not ideal: Violates principle of least privilege
- ⚠️ Use temporarily until container is updated

## Proper Solution (Recommended)

### What to Do in Your Container

Add this to your Dockerfile for `ghcr.io/kaladron/cpp-image/dev-env:latest`:

```dockerfile
# Create runner user matching GitHub Actions UID (1001)
RUN groupadd -g 1001 runner && \
    useradd -m -u 1001 -g runner -s /bin/bash runner && \
    echo "runner ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/runner && \
    chmod 0440 /etc/sudoers.d/runner

# Switch to runner user
USER runner
WORKDIR /home/runner
```

**Place this near the end of your Dockerfile**, after all system package installations.

### Bonus: Move Dependencies Into Container

Even better, add these to your Dockerfile to eliminate the "Install dependencies" CI step:

```dockerfile
RUN apt-get update && \
    apt-get install -y libsqlite3-dev && \
    ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
```

## After Container Update

Once you rebuild and push the container, update `.github/workflows/ci.yml`:

1. **Remove** the `options: --user root` line
2. **Either:**
   - Delete the "Install dependencies" step entirely (if you baked them in), OR
   - Add `sudo` before commands in that step

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
