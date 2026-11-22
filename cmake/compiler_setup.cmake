# SPDX-License-Identifier: Apache-2.0

# Enforce minimum supported toolchain: LLVM/Clang >= 22 with matching libc++ >= 22
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS "22.0")
  message(FATAL_ERROR
    "Galactic Bloodshed requires LLVM/Clang >= 22 with libc++ >= 22 "
    "(found ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}). "
    "Install clang-22 / libc++-22-dev or pass -DCMAKE_CXX_COMPILER=clang++-22."
  )
endif()

# Resolve symlinked compiler paths (e.g. /usr/bin/clang++-22 -> /usr/lib/llvm-22/bin/clang)
# so libc++ module headers, static archives, and lld match the active Clang version.
get_filename_component(_REAL_CXX_COMPILER "${CMAKE_CXX_COMPILER}" REALPATH)
get_filename_component(_CLANG_BIN_DIR "${_REAL_CXX_COMPILER}" DIRECTORY)
get_filename_component(_LLVM_PREFIX "${_CLANG_BIN_DIR}" DIRECTORY)

# Verify matching libc++ >= 22 headers are present alongside the active Clang installation
set(_GB_LIBCPP_CONFIG_CANDIDATES
  "${_LLVM_PREFIX}/include/c++/v1/__config"
  "/usr/include/c++/v1/__config"
)
set(_GB_LIBCPP_VERSION "")
foreach(_cfg IN LISTS _GB_LIBCPP_CONFIG_CANDIDATES)
  if(EXISTS "${_cfg}")
    file(STRINGS "${_cfg}" _libcpp_ver_line REGEX "^#[ \t]*define[ \t]+_LIBCPP_VERSION[ \t]+[0-9]+")
    if(_libcpp_ver_line MATCHES "([0-9]+)$")
      set(_GB_LIBCPP_VERSION "${CMAKE_MATCH_1}")
      break()
    endif()
  endif()
endforeach()
if(_GB_LIBCPP_VERSION AND _GB_LIBCPP_VERSION LESS 220000)
  message(FATAL_ERROR
    "Galactic Bloodshed requires libc++ >= 22 (_LIBCPP_VERSION >= 220000), "
    "but found _LIBCPP_VERSION=${_GB_LIBCPP_VERSION}."
  )
endif()

# Use LLVM libc++ statically linked
if(NOT CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
if(EXISTS "${_LLVM_PREFIX}/lib/libc++.a")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${_LLVM_PREFIX}/lib")
endif()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib++ -static-libgcc -Wl,-Bstatic -lc++ -lc++abi -Wl,-Bdynamic")

# Add libc++ module directory as a system include directory so Clang and Clang-Tidy
# treat 'import std;' declarations as system headers rather than user AST nodes.
if(EXISTS "${_LLVM_PREFIX}/share/libc++/v1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${_LLVM_PREFIX}/share/libc++/v1")
elseif(EXISTS "/lib/share/libc++/v1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem /lib/share/libc++/v1")
endif()

# Prefer matching lld linker if available
find_program(LLD_LINKER NAMES ld.lld lld HINTS "${_CLANG_BIN_DIR}")
if(LLD_LINKER)
    message(STATUS "Using lld linker: ${LLD_LINKER}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
else()
    message(STATUS "lld linker not found, using default linker")
endif()

enable_testing()

# Export compile commands by default for tooling (clangd, clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compilation database" FORCE)

# Set language version used



# Set compiler flags for different build types
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE")
endif()

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address -fno-omit-frame-pointer"
)
set(CMAKE_LINKER_FLAGS_DEBUG
    "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=undefined,address")

# Compiler options
string(JOIN " " GB_WARNING_FLAGS
    -Wall
    -Wextra
    -Wno-sign-compare
    -Wdocumentation
    -Wno-missing-designated-field-initializers
    -Wnull-dereference
    -Wpessimizing-move
    -Wredundant-move
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wsuggest-override
    -Wcomma
    -Wextra-semi
    -Wzero-as-null-pointer-constant
    -Wformat=2
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GB_WARNING_FLAGS}")

# Coverage support
option(ENABLE_COVERAGE "Enable code coverage testing" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage instrumentation")
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping -fcoverage-mcdc)
    add_link_options(-fprofile-instr-generate)
endif()
