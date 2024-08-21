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

FetchContent_Declare(
  scn
  GIT_REPOSITORY https://github.com/eliaskosunen/scnlib.git
  GIT_TAG v3.0.1
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(scn)

if(0)
  find_program(CLANG_TIDY clang-tidy)
  if(CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY};-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
  endif()
endif()

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
add_executable(makeuniv creator/makeplanet.cc creator/makestar.cc
                        creator/makeuniv.cc)
target_link_libraries(makeuniv PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

add_executable(enrol enrol.cc)
target_link_libraries(enrol PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

add_executable(racegen GB_racegen.cc enroll.cc racegen.cc)
target_link_libraries(racegen PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze)

add_library(gblib)
target_sources(
  gblib
  PUBLIC FILE_SET
         CXX_MODULES
         FILES
         gblib.cppm
         gblib-files_shl.cppm
         gblib-fire.cppm
         gblib-globals.cppm
         gblib-misc.cppm
         gblib-place.cppm
         gblib-planet.cppm
         gblib-race.cppm
         utils/gblib-rand.cppm
         gblib-sector.cppm
         gblib-ships.cppm
         gblib-shlmisc.cppm
         gblib-shootblast.cppm
         sql/gblib-sql.cppm
         gblib-star.cppm
         gblib-tele.cppm
         gblib-tweakables.cppm
         gblib-types.cppm
  PRIVATE files_shl.cc
          fire.cc
          load.cc
          misc.cc
          planet.cc
          utils/rand.cc
          sectormap.cc
          ships.cc
          shlmisc.cc
          shootblast.cc
          sql/sql.cc
          star.cc
          tele.cc
          victory.cc
          VN.cc)

add_library(commands)
target_sources(
  commands
  PUBLIC FILE_SET CXX_MODULES FILES commands/commands.cppm
  PRIVATE commands/analysis.cc
          commands/announce.cc
          commands/arm.cc
          commands/autoreport.cc
          commands/bid.cc
          commands/bless.cc
          commands/block.cc
          commands/bombard.cc
          commands/build.cc
          commands/capital.cc
          commands/capture.cc
          commands/center.cc
          commands/colonies.cc
          commands/cs.cc
          commands/declare.cc
          commands/defend.cc
          commands/detonate.cc
          commands/dissolve.cc
          commands/distance.cc
          commands/dock.cc
          commands/dump.cc
          commands/enslave.cc
          commands/examine.cc
          commands/explore.cc
          commands/fire.cc
          commands/fix.cc
          commands/give.cc
          commands/governors.cc
          commands/grant.cc
          commands/highlight.cc
          commands/insurgency.cc
          commands/invite.cc
          commands/jettison.cc
          commands/land.cc
          commands/launch.cc
          commands/load.cc
          commands/make_mod.cc
          commands/map.cc
          commands/mobilize.cc
          commands/motto.cc
          commands/mount.cc
          commands/move_popn.cc
          commands/name.cc
          commands/orbit.cc
          commands/order.cc
          commands/page.cc
          commands/pay.cc
          commands/personal.cc
          commands/pledge.cc
          commands/power.cc
          commands/production.cc
          commands/profile.cc
          commands/proj_fuel.cc
          commands/read_messages.cc
          commands/relation.cc
          commands/repair.cc
          commands/route.cc
          commands/rst.cc
          commands/scrap.cc
          commands/sell.cc
          commands/send_message.cc
          commands/star_locations.cc
          commands/survey.cc
          commands/tax.cc
          commands/technology.cc
          commands/tech_status.cc
          commands/toggle.cc
          commands/toxicity.cc
          commands/transfer.cc
          commands/treasury.cc
          commands/unpledge.cc
          commands/upgrade.cc
          commands/victory.cc
          commands/vote.cc
          commands/walk.cc
          commands/whois.cc
          commands/zoom.cc)
target_link_libraries(commands PRIVATE gblib ${SQLite3_LIBRARIES} glaze::glaze
                                       scn::scn)

set(OLD_LIBGBLIB_FILES
    bombard.cc
    build.cc
    doplanet.cc
    dosector.cc
    doship.cc
    doturncmd.cc
    fuel.cc
    map.cc
    move.cc
    moveplanet.cc
    moveship.cc
    order.cc
    prof.cc)

add_executable(GB GB_server.cc ${OLD_LIBGBLIB_FILES})
target_link_libraries(GB PRIVATE gblib commands ${SQLite3_LIBRARIES}
                                 glaze::glaze)

# Test targets
add_executable(shlmisc_test shlmisc_test.cc)
target_link_libraries(shlmisc_test PRIVATE gblib ${SQLite3_LIBRARIES})
add_test(NAME GB_shlmisc_test COMMAND shlmisc_test)

add_executable(gblib_test gblib_test.cc)
target_link_libraries(gblib_test PRIVATE gblib ${SQLite3_LIBRARIES})
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
