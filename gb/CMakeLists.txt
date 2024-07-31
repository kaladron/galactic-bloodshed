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

enable_testing()

project("galactic-bloodshed" LANGUAGES CXX)

# Require dot, treat the other components as optional
find_package(Doxygen REQUIRED dot)

# Bring in
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(glaze)

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

set(COMMON_FILES files_rw.cc files_shl.cc sql/sql.cc)

add_executable(
  makeuniv creator/makeplanet.cc creator/makestar.cc creator/makeuniv.cc
           creator/namegen.cc sectormap.cc ${COMMON_FILES})
target_link_libraries(makeuniv PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

add_executable(enrol enrol.cc sectormap.cc ${COMMON_FILES})
target_link_libraries(enrol PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

add_executable(racegen GB_racegen.cc enroll.cc racegen.cc sectormap.cc
                       ${COMMON_FILES})
target_link_libraries(racegen PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

add_library(gblib)
target_sources(
  gblib
  PUBLIC FILE_SET
         CXX_MODULES
         FILES
         gblib.cppm
         gblib-globals.cppm
         gblib-planet.cppm
         gblib-race.cppm
         utils/gblib-rand.cppm
         gblib-sector.cppm
         gblib-star.cppm
         gblib-tweakables.cppm
         gblib-types.cppm
  PRIVATE utils/rand.cc sectormap.cc star.cc victory.cc)

add_library(commands)
target_sources(
  commands
  PUBLIC FILE_SET CXX_MODULES FILES commands/commands.cppm
  PRIVATE commands/analysis.cc
          commands/announce.cc
          commands/autoreport.cc
          commands/block.cc
          commands/capital.cc
          commands/capture.cc
          commands/center.cc
          commands/colonies.cc
          commands/declare.cc
          commands/dissolve.cc
          commands/distance.cc
          commands/dock.cc
          commands/enslave.cc
          commands/examine.cc
          commands/explore.cc
          commands/fix.cc
          commands/governors.cc
          commands/grant.cc
          commands/highlight.cc
          commands/invite.cc
          commands/mobilize.cc
          commands/orbit.cc
          commands/pledge.cc
          commands/power.cc
          commands/production.cc
          commands/relation.cc
          commands/star_locations.cc
          commands/tax.cc
          commands/tech_status.cc
          commands/technology.cc
          commands/repair.cc
          commands/rst.cc
          commands/scrap.cc
          commands/survey.cc
          commands/toggle.cc
          commands/toxicity.cc
          commands/unpledge.cc
          commands/victory.cc
          commands/vote.cc
          commands/zoom.cc)
target_link_libraries(commands PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

set(OLD_LIBGBLIB_FILES
    VN.cc
    bombard.cc
    build.cc
    cs.cc
    doplanet.cc
    dosector.cc
    doship.cc
    doturncmd.cc
    fire.cc
    fuel.cc
    land.cc
    launch.cc
    load.cc
    map.cc
    max.cc
    move.cc
    moveplanet.cc
    moveship.cc
    name.cc
    order.cc
    place.cc
    prof.cc
    ships.cc
    shlmisc.cc
    shootblast.cc
    tech.cc
    tele.cc)

add_executable(GB GB_server.cc ${COMMON_FILES} ${OLD_LIBGBLIB_FILES})
target_link_libraries(GB PRIVATE gblib commands ${SQLite3_LIBRARIES}
                                 glaze::glaze)

# Test targets
add_executable(shlmisc_test shlmisc_test.cc)
target_link_libraries(shlmisc_test PRIVATE gblib)
add_test(NAME GB_shlmisc_test COMMAND shlmisc_test)

add_executable(gblib_test gblib_test.cc)
target_link_libraries(gblib_test PRIVATE gblib)
add_test(NAME GB_gblib_test COMMAND gblib_test)

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

doxygen_add_docs(gb-docs)
