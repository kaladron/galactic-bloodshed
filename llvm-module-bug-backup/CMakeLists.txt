cmake_minimum_required(VERSION 3.28)

# Enable experimental std module support BEFORE project()
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # Required for import std

# Use libc++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-lc++abi")

project(llvm_module_bug CXX)

# Require LLVM/Clang
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "This test requires Clang/LLVM compiler")
endif()

# Module library that wraps a header library
add_library(header_wrapper)
target_sources(header_wrapper
  PUBLIC
    FILE_SET CXX_MODULES FILES
      header_wrapper.cppm
)
target_include_directories(header_wrapper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Executable that imports both the wrapper and std
add_executable(test_import_conflict test.cc)
target_link_libraries(test_import_conflict PRIVATE header_wrapper)
