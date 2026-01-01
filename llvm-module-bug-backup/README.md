# LLVM C++ Modules Bug: Conflict between Header-Wrapped Modules and `import std`

## Summary

When a C++ module wraps header libraries using the `module;` + `#include` pattern, and those headers transitively include specific combinations of standard library headers (particularly `<format>` + `<iostream>` + `<string>`), subsequently using `import std;` and `std::println()` or `std::format()` in the same translation unit causes compilation errors with incomplete types and conflicting definitions.

## Minimal Reproduction

A module that wraps headers containing:
```cpp
#include <format>
#include <iostream>
#include <string>
```

Then in a file that imports both this module and `import std;`, using `std::println()` fails to compile.

**Note:** Individual headers (`<format>` alone, `<iostream>` alone) don't trigger the bug. The issue appears when certain combinations are wrapped together in a module.

## Expected Behavior

A translation unit should be able to:
1. Import a module that wraps header libraries (containing `#include <format>`, `<iostream>`, `<string>`)
2. Also `import std;` for direct use of standard library features
3. Use `std::println()` and `std::format()` without conflicts

## Actual Behavior

Compilation fails with errors like:
- `call to implicitly-deleted default constructor of 'formatter<std::__1::basic_format_string<char>, char>'`
- `no member named 'parse' in 'std::formatter<std::__1::basic_format_string<char>>'`
- `constexpr variable '__handles_' must be initialized by a constant expression`

## Root Cause

The module that wraps headers brings in standard library components via traditional `#include` directives. When `import std;` is used later, it imports the same components from the module interface, creating ODR violations and incomplete type definitions.

## Reproduction

### Prerequisites
- LLVM/Clang 22+ with C++ modules support
- CMake 3.28+
- libc++ (LLVM's C++ standard library)

### Build and Run

```bash
cmake -S . -B build
cmake --build build
```

### Expected Result
Build should succeed and program should run, printing:
```
Testing std::println after importing header_wrapper
Message: Hello
```

### Actual Result
Build fails with std::format-related errors.

## Environment

- Compiler: clang++-22 (LLVM)
- Standard Library: libc++
- C++ Standard: C++26
- Platform: Linux (Ubuntu 24.04)

## Files

- `CMakeLists.txt` - Build configuration (requires CMake 3.28+, Ninja generator)
- `simple_header_lib.h` - Minimal header library using `<format>`, `<iostream>`, and `<string>`
- `header_wrapper.cppm` - Module wrapping the header library using `module;` + `#include`
- `test.cc` - Test file demonstrating the issue: imports `header_wrapper` and `std`, uses `std::println()`

## Observations

1. **Individual headers work fine**: Wrapping just `<format>` or just `<iostream>` doesn't trigger the bug
2. **Combination triggers bug**: `<format>` + `<iostream>` + `<string>` together causes the failure
3. **Real-world impact**: Libraries like Boost.Asio that include many stdlib headers trigger this issue
4. **Affects formatting**: Specifically impacts `std::format` and `std::println` usage

## Workarounds

Currently known workarounds:
1. Don't use `import std;` in files that import header-wrapped modules
2. Use traditional `#include <iostream>` instead of module imports (defeats the purpose)
3. Avoid `std::format` and `std::println` in affected files

## Impact

This significantly limits the usefulness of C++ modules when interfacing with existing header-only libraries, which is a common real-world scenario. Libraries like Boost.Asio, header-only JSON libraries, etc., cannot be cleanly wrapped as modules without preventing the use of `import std;`.

## Related

This issue affects any project that:
- Wraps third-party header libraries as modules
- Wants to use both wrapped libraries and `import std;` in the same translation unit
- Uses complex standard library templates like `std::format`, `std::ranges`, etc.
