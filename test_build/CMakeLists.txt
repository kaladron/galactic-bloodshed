# SPDX-License-Identifier: Apache-2.0
# 
# Standalone CMakeLists.txt for building the glaze char bug test

cmake_minimum_required(VERSION 3.25)

project(glaze_char_bug_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Fetch glaze library with explicit source directory
include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/glaze-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/glaze-build
)
FetchContent_MakeAvailable(glaze)

# Create test executable
add_executable(glaze_char_bug_test ../glaze_char_bug_test.cpp)
target_link_libraries(glaze_char_bug_test PRIVATE glaze::glaze)