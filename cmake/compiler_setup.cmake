# SPDX-License-Identifier: Apache-2.0

# Use LLVM libc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-lc++abi")

# Prefer lld linker if available
find_program(LLD_LINKER NAMES ld.lld lld)
if(LLD_LINKER)
    message(STATUS "Using lld linker: ${LLD_LINKER}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
else()
    message(STATUS "lld linker not found, using default linker")
endif()

enable_testing()

# Set language version used



# Set compiler flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address -fno-omit-frame-pointer"
)
set(CMAKE_LINKER_FLAGS_DEBUG
    "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=undefined,address")

# Compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-compare -Wdocumentation -Wno-missing-designated-field-initializers")
