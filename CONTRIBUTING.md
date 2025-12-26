# Contributing to Galactic Bloodshed

Thank you for contributing to Galactic Bloodshed!

## First-time Setup

**⚠️ Important:** After cloning the repository, please install git hooks:

```bash
./tools/install-hooks.sh
```

This installs a pre-commit hook that checks code formatting before commits.

## Code Formatting

We use clang-format to maintain consistent code style.

**Check if your code is formatted correctly:**
```bash
ninja -C build format
```

**Fix formatting issues:**
```bash
ninja -C build format-fix
```

**Note:** The pre-commit hook will check this automatically if you've run `install-hooks.sh`.

## Development Workflow

1. Install hooks: `./tools/install-hooks.sh` (first time only)
2. Make your changes
3. Check formatting: `ninja -C build format-fix`
4. Commit your changes (hook will verify formatting)
5. Push and open a PR

## Building and Testing

```bash
# Configure
cmake -S . -B build

# Build
cmake --build build

# Run tests
(cd build && ctest)
```

## Questions?

See [AGENTS.md](AGENTS.md) for detailed architecture and coding guidelines.
