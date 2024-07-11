# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

# Enables the Standard module support. This needs to be done before selecting
# the languages.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Use LLVM libc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-lc++abi")

project("galactic-bloodshed" LANGUAGES CXX)

# Set language version used

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
# Currently CMake requires extensions enabled when using import std.
# https://gitlab.kitware.com/cmake/cmake/-/issues/25916
# https://gitlab.kitware.com/cmake/cmake/-/issues/25539
set(CMAKE_CXX_EXTENSIONS ON)

# Include GNUInstallDirs to get canonical paths
include(GNUInstallDirs)

# Compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-compare")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/.. ${CMAKE_SOURCE_DIR}/sql)

# Attempt to find SQLite3 on the system
find_package(SQLite3 REQUIRED)

# Libraries

# Executable targets

set(UTILS_FILES utils/rand.cc)
set(COMMON_FILES files.cc files_rw.cc files_shl.cc sql/sql.cc ${UTILS_FILES})

add_executable(
  makeuniv creator/makeplanet.cc creator/makestar.cc creator/makeuniv.cc
           creator/namegen.cc sectormap.cc ${COMMON_FILES})
target_link_libraries(makeuniv PRIVATE gblib ${SQLite3_LIBRARIES})

add_executable(enrol enrol.cc sectormap.cc ${COMMON_FILES})
target_link_libraries(enrol PRIVATE gblib ${SQLite3_LIBRARIES})

add_executable(racegen GB_racegen.cc enroll.cc racegen.cc sectormap.cc ${COMMON_FILES})
target_link_libraries(racegen PRIVATE gblib ${SQLite3_LIBRARIES})

add_library(gblib)
target_sources(gblib PUBLIC FILE_SET CXX_MODULES FILES gblib.cppm)

# Configuration variable for PKGDATADIR
set(PKGDATADIR
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}/"
    CACHE STRING "Path to the package data directory")
add_definitions(-DPKGDATADIR="${PKGDATADIR}")

# Set PKGSTATEDIR using CMAKE_INSTALL_LOCALSTATEDIR for local state files
set(PKGSTATEDIR
    "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/${PROJECT_NAME}/"
    CACHE STRING "Path to the package state directory")
add_definitions(-DPKGSTATEDIR="${PKGSTATEDIR}")

# Set DOCDIR using CMAKE_INSTALL_DOCDIR for documentation files
set(DOCDIR
    "${CMAKE_INSTALL_FULL_DOCDIR}/${PROJECT_NAME}/"
    CACHE STRING "Path to the package document directory")
add_definitions(-DDOCDIR="${DOCDIR}")
