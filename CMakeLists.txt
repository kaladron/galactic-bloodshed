# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.31)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enables the Standard module support. This needs to be done before selecting
# the languages.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
# Currently CMake requires extensions enabled when using import std.
# https://gitlab.kitware.com/cmake/cmake/-/issues/25916
# https://gitlab.kitware.com/cmake/cmake/-/issues/25539
set(CMAKE_CXX_EXTENSIONS ON)

# Use LLVM libc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-lc++abi")

project("galactic-bloodshed" LANGUAGES CXX)

# Include GNUInstallDirs to get canonical paths
include(GNUInstallDirs)


include(cmake/compiler_setup.cmake)

add_subdirectory(gb)
add_subdirectory(data)
add_subdirectory(docs)

