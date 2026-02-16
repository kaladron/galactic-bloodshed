# Container Fix Guide: Proper Permissions for GitHub Actions

## TL;DR - What You Need to Do

**YES, you should fix this in the container instead of using `--user root`!**

Your container **already creates a user** with proper sudo access, but uses **UID 1000** instead of the **UID 1001** that GitHub Actions expects.

## The Problem

Your container's Dockerfile has:
```dockerfile
ARG USER_UID=1000  # ← GitHub Actions needs 1001
ARG USER_GID=$USER_UID

# ... creates vscode user with UID 1000 ...
USER $USERNAME  # ← Runs as UID 1000
```

GitHub Actions mounts workspace directories with UID 1001, causing permission conflicts. The current workaround (`--user root`) works but violates the principle of least privilege.

## Solution Options

### Option 1: Change Default UID to 1001 (Recommended - Easiest!)

**Simplest fix!** Just change one line in your Dockerfile:

```dockerfile
# BEFORE:
ARG USER_UID=1000

# AFTER:
ARG USER_UID=1001
```

**That's it!** Your existing user creation logic will use UID 1001. Rebuild and push.

**Why this is best:**
- ✅ Minimal change (1 line)
- ✅ Works for both GitHub Actions and local dev
- ✅ Keeps all your existing user setup
- ✅ No changes to devcontainer needed

### Option 2: Build with --build-arg

Keep your Dockerfile as-is, but when building for CI, use:

```bash
docker build \
  --build-arg USER_UID=1001 \
  -t ghcr.io/kaladron/cpp-image/dev-env:latest \
  .
```

**Why use this:**
- ✅ Doesn't change default UID (stays 1000 for local dev)
- ⚠️ Need to remember build arg every time
- ⚠️ May confuse developers who rebuild locally

### Option 3: Use --user 1001 in Workflow

Don't change the container, but in `.github/workflows/ci.yml`:

```yaml
container:
  image: ghcr.io/kaladron/cpp-image/dev-env:latest
  credentials:
    username: ${{ github.actor }}
    password: ${{ secrets.GITHUB_TOKEN }}
  options: --user 1001:1001  # ← Changed from "root"
```

**Why use this:**
- ✅ No container rebuild needed
- ✅ Safer than `--user root`
- ⚠️ Container still has UID 1000 user, but we override it
- ⚠️ May cause issues if scripts assume `$HOME` or username

### Option 4: Keep Using Root (Not Recommended)

The current workaround works but violates security best practices:

```yaml
options: --user root  # ← Current workaround
```

**Why avoid this:**
- ❌ Violates principle of least privilege
- ❌ Security risk if any step has vulnerabilities
- ⚠️ Only use as temporary solution

## Bonus Optimization: Bake Dependencies Into Container

Your CI workflow runs these steps on **every** CI run:

```bash
apt-get update
apt-get install -y libsqlite3-dev
ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld
ln -sf /usr/bin/lld-22 /usr/bin/lld
ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps
ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format
mkdir -p /lib/share/libc++
ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1
```

**Move these into your Dockerfile** for much faster CI!

Add this to your Dockerfile, **after the LLVM installation and before the USER switch**:

```dockerfile
# Install project-specific dependencies and create symlinks
RUN apt-get update && \
    apt-get install -y --no-install-recommends libsqlite3-dev && \
    # Create symlinks for LLVM tools
    ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    # Create C++ standard library modules symlink
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1 && \
    # Clean up to reduce image size
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# This should go BEFORE your existing USER $USERNAME line
```

Then **delete the entire "Install dependencies" step** from your CI workflow!

## Complete Dockerfile Example

Here's what your Dockerfile should look like with all recommendations:

```dockerfile
# ... your existing content up to LLVM installation ...

# Install LLVM apt repository and full clang 22 toolchain plus ninja-build
# ... your existing LLVM installation code ...

# Install project-specific dependencies and create symlinks
RUN apt-get update && \
    apt-get install -y --no-install-recommends libsqlite3-dev && \
    ln -sf /usr/bin/ld.lld-22 /usr/bin/ld.lld && \
    ln -sf /usr/bin/lld-22 /usr/bin/lld && \
    ln -sf /usr/bin/clang-scan-deps-22 /usr/bin/clang-scan-deps && \
    ln -sf /usr/bin/clang-format-22 /usr/bin/clang-format && \
    mkdir -p /lib/share/libc++ && \
    ln -sf /usr/lib/llvm-22/share/libc++/v1 /lib/share/libc++/v1 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# ... your existing oh-my-zsh installation ...

# Update CA certificates
RUN update-ca-certificates

# Change default UID to match GitHub Actions
ARG USERNAME=vscode
ARG USER_UID=1001  # ← Changed from 1000
ARG USER_GID=$USER_UID

# ... rest of your existing user creation code stays the same ...

USER $USERNAME
```

## After Container Update

### If Using Option 1 (Changed UID to 1001):

Update `.github/workflows/ci.yml`:

```yaml
container:
  image: ghcr.io/kaladron/cpp-image/dev-env:latest
  credentials:
    username: ${{ github.actor }}
    password: ${{ secrets.GITHUB_TOKEN }}
  # Remove: options: --user root
```

If you baked dependencies in:
```yaml
steps:
  - name: Checkout code
    uses: actions/checkout@v4
  
  # DELETE the "Install dependencies" step entirely!
  
  - name: Configure CMake
    # ... rest of your workflow ...
```

If dependencies aren't baked in, add `sudo`:
```yaml
- name: Install dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y libsqlite3-dev
    # ... etc
```

### If Using Option 2 (Build Arg):

Same as Option 1, but remember to build with `--build-arg USER_UID=1001`.

### If Using Option 3 (--user 1001):

Just change the one line in ci.yml:
```yaml
options: --user 1001:1001  # Changed from "root"
```

Keep the "Install dependencies" step as-is (no sudo needed since running as root-like).

## Testing Your Container Changes Locally

Before pushing to the registry:

```bash
# Build the container (with UID 1001 if using Option 1 or 2)
docker build --build-arg USER_UID=1001 -t test-container .

# Test that it runs as UID 1001
docker run --rm test-container id
# Should output: uid=1001(vscode) gid=1001(vscode) groups=1001(vscode)

# Test file permissions work
docker run --rm -v $(pwd):/workspace test-container bash -c "cd /workspace && touch test.txt && rm test.txt"
# Should succeed without permission errors

# Test building your project
docker run --rm -v $(pwd):/workspace -w /workspace test-container bash -c "cmake -B build && cmake --build build"
```

## Benefits of Proper Solution

1. **Security**: Follows principle of least privilege - no root access needed
2. **Speed**: No package installation on every CI run (if dependencies baked in)
3. **Simplicity**: Cleaner CI workflow with fewer steps
4. **Consistency**: Same environment everywhere (CI, devcontainer, local)
5. **Debugging**: Easier to reproduce CI issues locally
6. **Maintainability**: Fewer moving parts in the workflow

## Migration Checklist

- [ ] Decide which option (1, 2, or 3) to use
- [ ] If Option 1: Change `ARG USER_UID=1001` in Dockerfile
- [ ] If Option 2: Update build command to use `--build-arg USER_UID=1001`
- [ ] **Recommended**: Add libsqlite3-dev and symlinks to Dockerfile
- [ ] Rebuild container: `docker build -t ghcr.io/kaladron/cpp-image/dev-env:latest .`
- [ ] Test locally (see commands above)
- [ ] Push to registry: `docker push ghcr.io/kaladron/cpp-image/dev-env:latest`
- [ ] Update `.github/workflows/ci.yml` (remove `--user root` or change to `--user 1001:1001`)
- [ ] If dependencies baked in: Delete "Install dependencies" step
- [ ] If dependencies not baked in: Add `sudo` before commands
- [ ] Test: Push a commit and verify CI passes
- [ ] Update `.devcontainer/devcontainer.json` if needed (should work automatically)

## Recommendation

**Use Option 1** (change default UID to 1001) + bake in dependencies. This gives you:
- Simplest implementation
- Best security
- Fastest CI
- Most consistent across environments

## Questions?

If you have questions about any of these options, feel free to ask!
