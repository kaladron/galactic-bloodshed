# SPDX-License-Identifier: Apache-2.0

# Configuration variable for PKGDATADIR
set(PKGDATADIR
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/"
    CACHE STRING "Path to the package data directory")
add_definitions(-DPKGDATADIR="${PKGDATADIR}")

# Set PKGSTATEDIR using CMAKE_INSTALL_LOCALSTATEDIR for local state files
set(PKGSTATEDIR
    "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/${CMAKE_PROJECT_NAME}/"
    CACHE STRING "Path to the package state directory")
add_definitions(-DPKGSTATEDIR="${PKGSTATEDIR}")
install(DIRECTORY DESTINATION ${PKGSTATEDIR})

# Set DOCDIR using CMAKE_INSTALL_DOCDIR for documentation files
set(DOCDIR
    "${CMAKE_INSTALL_FULL_DOCDIR}/"
    CACHE STRING "Path to the package document directory")
add_definitions(-DDOCDIR="${DOCDIR}")

# Require dot, treat the other components as optional
find_package(Doxygen REQUIRED dot)

# Bring in
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
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

# Include directories
include_directories(.. sql)

# Attempt to find SQLite3 on the system
find_package(SQLite3 REQUIRED)

# Libraries

# Executable targets
add_executable(makeuniv creator/makeplanet.cc creator/makestar.cc
                        creator/makeuniv.cc)
target_link_libraries(makeuniv PRIVATE gblib SQLite::SQLite3 glaze::glaze)

add_executable(enrol enrol.cc)
target_link_libraries(enrol PRIVATE gblib SQLite::SQLite3 glaze::glaze)

add_executable(racegen GB_racegen.cc enroll.cc racegen.cc)
target_link_libraries(racegen PRIVATE gblib SQLite::SQLite3 glaze::glaze)

add_library(gblib)
target_sources(
  gblib
  PUBLIC FILE_SET
         CXX_MODULES
         FILES
         gblib.cppm
         gblib-bombard.cppm
         gblib-build.cppm
         gblib-doplanet.cppm
         gblib-dosector.cppm
         gblib-doship.cppm
         gblib-doturncmd.cppm
         gblib-files_shl.cppm
         gblib-fire.cppm
         gblib-fuel.cppm
         gblib-globals.cppm
         gblib-map.cppm
         gblib-misc.cppm
         gblib-move.cppm
         gblib-order.cppm
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
  PRIVATE bombard.cc
          build.cc
          doplanet.cc
          dosector.cc
          doship.cc
          doturncmd.cc
          files_shl.cc
          fire.cc
          fuel.cc
          load.cc
          map.cc
          misc.cc
          move.cc
          order.cc
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
target_link_libraries(gblib PUBLIC glaze::glaze)

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
target_link_libraries(commands PRIVATE gblib SQLite::SQLite3 glaze::glaze
                                       scn::scn)

add_executable(GB GB_server.cc)
target_link_libraries(GB PRIVATE gblib commands SQLite::SQLite3 glaze::glaze)
install(TARGETS GB)

# Test targets
add_executable(shlmisc_test shlmisc_test.cc)
target_link_libraries(shlmisc_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_shlmisc_test COMMAND shlmisc_test)

add_executable(gblib_test gblib_test.cc)
target_link_libraries(gblib_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_gblib_test COMMAND gblib_test)

add_executable(move_test move_test.cc)
target_link_libraries(move_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_move_test COMMAND move_test)

add_executable(dosector_simple_test dosector_simple_test.cc)
target_link_libraries(dosector_simple_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_dosector_simple_test COMMAND dosector_simple_test)

add_executable(race_serialization_test race_serialization_test.cc)
target_link_libraries(race_serialization_test PRIVATE gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_race_serialization_test COMMAND race_serialization_test)

add_executable(race_sqlite_test race_sqlite_test.cc)
target_link_libraries(race_sqlite_test PRIVATE gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_race_sqlite_test COMMAND race_sqlite_test)

doxygen_add_docs(gb-docs)
