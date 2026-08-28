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


set(CMAKE_CXX_FLAGS "-stdlib=libc++")

project("galactic-bloodshed" VERSION 0.1.0 LANGUAGES CXX)

# Include GNUInstallDirs to get canonical paths
include(GNUInstallDirs)

# Set default build type to Debug for development if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


include(cmake/compiler_setup.cmake)

# clang-tidy target for running static analysis with auto-fix
find_program(
  RUN_CLANG_TIDY_EXE
  NAMES run-clang-tidy run-clang-tidy-22 run-clang-tidy-21 run-clang-tidy-20 run-clang-tidy-19 run-clang-tidy.py
)
if(RUN_CLANG_TIDY_EXE)
  add_custom_target(
    tidy
    COMMAND ${RUN_CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}
      "-header-filter=.*(gb|dallib)/.*"
      "-exclude-header-filter=.*third_party/.*"
      -quiet
      "(gb|dallib)/.*\\.(cc|cppm)$"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy..."
    VERBATIM
  )
  add_custom_target(
    tidy-fix
    COMMAND ${RUN_CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}
      "-header-filter=.*(gb|dallib)/.*"
      "-exclude-header-filter=.*third_party/.*"
      -quiet
      -fix
      "(gb|dallib)/.*\\.(cc|cppm)$"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy with --fix..."
    VERBATIM
  )
  add_custom_target(
    tidy-changed
    COMMAND ${CMAKE_SOURCE_DIR}/tools/tidy-changed.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy on changed files..."
    VERBATIM
  )
  add_custom_target(
    tidy-changed-fix
    COMMAND ${CMAKE_SOURCE_DIR}/tools/tidy-changed.sh -fix
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy with --fix on changed files..."
    VERBATIM
  )
  add_custom_target(
    tidy-full
    COMMAND ${RUN_CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}
      "-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy-full"
      "-header-filter=.*(gb|dallib)/.*"
      "-exclude-header-filter=.*third_party/.*"
      -quiet
      "(gb|dallib)/.*\\.(cc|cppm)$"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running exhaustive clang-tidy analysis..."
    VERBATIM
  )
  add_custom_target(
    tidy-full-fix
    COMMAND ${RUN_CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}
      "-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy-full"
      "-header-filter=.*(gb|dallib)/.*"
      "-exclude-header-filter=.*third_party/.*"
      -quiet
      -fix
      "(gb|dallib)/.*\\.(cc|cppm)$"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running exhaustive clang-tidy analysis with --fix..."
    VERBATIM
  )
else()
  message(STATUS "run-clang-tidy not found, 'tidy', 'tidy-fix', 'tidy-full' targets disabled")
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

add_subdirectory(third_party)
add_subdirectory(gb)
add_subdirectory(data)
add_subdirectory(docs)
add_subdirectory(help)

# ==============================================================================
# Packaging (CPack)
# ==============================================================================
set(CPACK_PACKAGE_NAME "galactic-bloodshed")
set(CPACK_PACKAGE_VENDOR "Galactic Bloodshed Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Galactic Bloodshed 4X multiplayer space strategy game server")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/kaladron/galactic-bloodshed")
set(CPACK_PACKAGE_CONTACT "Jeff Bailey <jeffbailey@google.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

if(NOT CPACK_PACKAGE_VERSION)
  if(PROJECT_VERSION)
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  else()
    set(CPACK_PACKAGE_VERSION "0.1.0")
  endif()
endif()

# Debian-specific packaging options
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jeff Bailey <jeffbailey@google.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Galactic Bloodshed 4X multiplayer space strategy game server")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/kaladron/galactic-bloodshed")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# Source package options
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "galactic-bloodshed-${CPACK_PACKAGE_VERSION}-source")
set(CPACK_SOURCE_IGNORE_FILES
  "/\\.git/"
  "/build/"
  "/\\.cache/"
  "/\\.clangd/"
  ".*~$"
)

# Binary package options
set(CPACK_SET_DESTDIR ON)
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_PACKAGE_FILE_NAME "galactic-bloodshed-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)

