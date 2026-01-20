# SPDX-License-Identifier: Apache-2.0

# Third-party dependencies - warnings suppressed
# All targets in this directory are built with -w to suppress warnings
# FetchContent brings in external libraries, then we create module wrappers

# Bring in FetchContent
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v6.2.0
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(glaze)

FetchContent_Declare(
  scn
  GIT_REPOSITORY https://github.com/eliaskosunen/scnlib.git
  GIT_TAG v3.0.1
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(scn)

FetchContent_Declare(
  tabulate
  GIT_REPOSITORY https://github.com/p-ranav/tabulate.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(tabulate)

# Fetch Boost (includes Asio)
# PATCH_COMMAND strips standard library #includes for C++ module compatibility.
# Keeps #include <boost/...> and #include <openssl/...>, removes all other <...> includes.
FetchContent_Declare(
  Boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  EXCLUDE_FROM_ALL
  PATCH_COMMAND bash ${CMAKE_SOURCE_DIR}/cmake/strip_std_includes.sh
)
FetchContent_MakeAvailable(Boost)

# Suppress warnings on all FetchContent library targets
# These are external dependencies we don't control
set_target_properties(scn PROPERTIES COMPILE_OPTIONS "-w")

# Module wrapper libraries - suppress warnings on external code

# Create tabulate module wrapper library
add_library(tabulate_module)
target_sources(
  tabulate_module
  PUBLIC FILE_SET CXX_MODULES FILES tabulate.cppm)
target_link_libraries(tabulate_module PUBLIC tabulate::tabulate)
target_compile_options(tabulate_module PRIVATE -w)

# Create scnlib module wrapper library
add_library(scnlib_module)
target_sources(
  scnlib_module
  PUBLIC FILE_SET CXX_MODULES FILES scnlib.cppm)
target_link_libraries(scnlib_module PUBLIC scn::scn)
target_compile_options(scnlib_module PRIVATE -w)

# Create glaze JSON module wrapper library
add_library(glaze_json_module)
target_sources(
  glaze_json_module
  PUBLIC FILE_SET CXX_MODULES FILES glaze_json.cppm)
target_link_libraries(glaze_json_module PUBLIC glaze::glaze)
target_compile_options(glaze_json_module PRIVATE -w)

# Create glaze core module wrapper library (for customization templates)
add_library(glaze_core_module)
target_sources(
  glaze_core_module
  PUBLIC FILE_SET CXX_MODULES FILES glaze_core.cppm)
target_link_libraries(glaze_core_module PUBLIC glaze::glaze)
target_compile_options(glaze_core_module PRIVATE -w)

# Create glaze TOML module wrapper library
add_library(glaze_toml_module)
target_sources(
  glaze_toml_module
  PUBLIC FILE_SET CXX_MODULES FILES glaze_toml.cppm)
target_link_libraries(glaze_toml_module PUBLIC glaze::glaze)
target_compile_options(glaze_toml_module PRIVATE -w)

# Create asio_standalone interface library
add_library(asio_standalone INTERFACE)
target_link_libraries(asio_standalone INTERFACE Boost::headers)
target_include_directories(asio_standalone SYSTEM INTERFACE 
  ${Boost_SOURCE_DIR}/libs/asio/include
  ${Boost_SOURCE_DIR}/libs/config/include
  ${Boost_SOURCE_DIR}/libs/core/include
  ${Boost_SOURCE_DIR}/libs/system/include
  ${Boost_SOURCE_DIR}/libs/throw_exception/include
  ${Boost_SOURCE_DIR}/libs/assert/include
  ${Boost_SOURCE_DIR}/libs/predef/include
)
target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

# Create asio module wrapper library
add_library(asio_module)
target_sources(
  asio_module
  PUBLIC FILE_SET CXX_MODULES FILES asio.cppm)
target_link_libraries(asio_module PUBLIC asio_standalone)
target_compile_options(asio_module PRIVATE -w)

# Create strong_id module wrapper library
# NOTE: This is project code (will move to separate repo eventually), so warnings are enabled
add_library(strong_id_module)
target_sources(
  strong_id_module
  PUBLIC FILE_SET CXX_MODULES FILES strong_id.cppm)
