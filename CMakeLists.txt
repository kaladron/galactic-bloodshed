# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 4.0.0)

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

# Use LLVM libc only if clang
#if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-lc++abi")
set(CMAKE_LINKER_TYPE LLD)
#endif()

project("galactic-bloodshed" LANGUAGES CXX)

# Include GNUInstallDirs to get canonical paths
include(GNUInstallDirs)

# Set default build type to Debug for development if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


include(cmake/compiler_setup.cmake)

# clang-tidy target for running static analysis with auto-fix
find_program(RUN_CLANG_TIDY_EXE NAMES run-clang-tidy)
if(RUN_CLANG_TIDY_EXE)
  add_custom_target(
    tidy
    COMMAND ${RUN_CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}
      -header-filter='.*galactic-bloodshed.*'
      -quiet
      "gb/.*\\.\(cc|cppm\)$$"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy..."
    VERBATIM
  )
  add_custom_target(
    tidy-fix
    COMMAND ${RUN_CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}
      -header-filter='.*galactic-bloodshed.*'
      -quiet
      -fix
      "gb/.*\\.\(cc|cppm\)$$"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy with --fix..."
    VERBATIM
  )
else()
  message(STATUS "run-clang-tidy not found, 'tidy' and 'tidy-fix' targets disabled")
endif()

# clang-format target for formatting all source files
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
  file(GLOB_RECURSE ALL_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/gb/*.cc
    ${CMAKE_SOURCE_DIR}/gb/*.h
    ${CMAKE_SOURCE_DIR}/gb/*.cppm
  )
  add_custom_target(
    format
    COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror --style=file ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking clang-format compliance..."
    VERBATIM
  )
  add_custom_target(
    format-fix
    COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format on all source files..."
    VERBATIM
  )
else()
  message(STATUS "clang-format not found, 'format' and 'format-fix' targets disabled")
endif()

add_subdirectory(gb)
add_subdirectory(data)
add_subdirectory(docs)
add_subdirectory(help)

