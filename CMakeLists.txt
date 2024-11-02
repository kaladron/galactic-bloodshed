# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(compiler_setup)

project("galactic-bloodshed" LANGUAGES CXX)

# Include GNUInstallDirs to get canonical paths
include(GNUInstallDirs)

add_subdirectory(gb)

