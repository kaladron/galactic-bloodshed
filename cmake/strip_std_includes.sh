#!/bin/bash
# Strip standard library #includes from source files for C++ module compatibility.
# Keeps #include <boost/...> and #include <openssl/...>, removes all other <...> includes.

find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.ipp" \) \
    -exec sed -i '/^[[:space:]]*#[[:space:]]*include </{/boost/b;/openssl/b;d;}' {} +
