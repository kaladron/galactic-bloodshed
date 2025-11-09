# SPDX-License-Identifier: Apache-2.0

# Configuration variable for PKGDATADIR
# In Debug mode, use source directories for development
# In Release mode, use install directories for production
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Development mode: use source data directory
    set(PKGDATADIR
        "${CMAKE_SOURCE_DIR}/data/"
        CACHE STRING "Path to the package data directory")
    message(STATUS "Debug build: Using source data directory: ${PKGDATADIR}")
else()
    # Production mode: use install directory  
    set(PKGDATADIR
        "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/"
        CACHE STRING "Path to the package data directory")
    message(STATUS "Release build: Using install data directory: ${PKGDATADIR}")
endif()
add_definitions(-DPKGDATADIR="${PKGDATADIR}")

# Set PKGSTATEDIR using CMAKE_INSTALL_LOCALSTATEDIR for local state files
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Development mode: use build directory for database and state files
    set(PKGSTATEDIR
        "${CMAKE_BINARY_DIR}/gamedata/"
        CACHE STRING "Path to the package state directory")
    # Create the development state directory and subdirectories
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gamedata")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gamedata/News")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gamedata/Tele")
    message(STATUS "Debug build: Using build state directory: ${PKGSTATEDIR}")
else()
    # Production mode: use install directory
    set(PKGSTATEDIR
        "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/${CMAKE_PROJECT_NAME}/"
        CACHE STRING "Path to the package state directory")
    message(STATUS "Release build: Using install state directory: ${PKGSTATEDIR}")
endif()
add_definitions(-DPKGSTATEDIR="${PKGSTATEDIR}")
install(DIRECTORY DESTINATION ${PKGSTATEDIR})

# Set DOCDIR using CMAKE_INSTALL_DOCDIR for documentation files
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Development mode: use source docs directory
    set(DOCDIR
        "${CMAKE_SOURCE_DIR}/docs/"
        CACHE STRING "Path to the package document directory")
    message(STATUS "Debug build: Using source docs directory: ${DOCDIR}")
else()
    # Production mode: use install directory
    set(DOCDIR
        "${CMAKE_INSTALL_FULL_DOCDIR}/"
        CACHE STRING "Path to the package document directory")
    message(STATUS "Release build: Using install docs directory: ${DOCDIR}")
endif()
add_definitions(-DDOCDIR="${DOCDIR}")

# Require dot, treat the other components as optional
find_package(Doxygen OPTIONAL_COMPONENTS dot)

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

FetchContent_Declare(
  tabulate
  GIT_REPOSITORY https://github.com/p-ranav/tabulate.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(tabulate)

# Include directories
include_directories(..)

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

# Create dallib as a separate library for Data Access Layer
add_library(dallib)
target_sources(
  dallib
  PUBLIC FILE_SET CXX_MODULES FILES dal/dallib.cppm
  PRIVATE dal/database.cc dal/json_store.cc dal/schema.cc)
target_link_libraries(dallib PUBLIC SQLite::SQLite3)

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
         gblib-gameobj.cppm
         gblib-globals.cppm
         gblib-map.cppm
         gblib-misc.cppm
         gblib-move.cppm
         gblib-order.cppm
         gblib-place.cppm
         gblib-planet.cppm
         gblib-race.cppm
         utils/gblib-rand.cppm
         repositories/gblib-repositories.cppm
         services/gblib-services.cppm
         gblib-sector.cppm
         gblib-ships.cppm
         gblib-shlmisc.cppm
         gblib-shootblast.cppm
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
          services/entity_manager.cc
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
          star.cc
          tele.cc
          victory.cc
          VN.cc)
target_link_libraries(gblib PUBLIC glaze::glaze dallib)

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
                                       scn::scn tabulate::tabulate)

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

add_executable(race_sqlite_test race_sqlite_test.cc)
target_link_libraries(race_sqlite_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_race_sqlite_test COMMAND race_sqlite_test)

add_executable(sector_sqlite_test sector_sqlite_test.cc)
target_link_libraries(sector_sqlite_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_sector_sqlite_test COMMAND sector_sqlite_test)

add_executable(sectormap_sqlite_test sectormap_sqlite_test.cc)
target_link_libraries(sectormap_sqlite_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_sectormap_sqlite_test COMMAND sectormap_sqlite_test)

add_executable(stardata_json_test stardata_json_test.cc)
target_link_libraries(stardata_json_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_stardata_json_test COMMAND stardata_json_test)

add_executable(commod_json_test commod_json_test.cc)
target_link_libraries(commod_json_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_commod_json_test COMMAND commod_json_test)

add_executable(block_json_test block_json_test.cc)
target_link_libraries(block_json_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_block_json_test COMMAND block_json_test)

add_executable(power_json_test power_json_test.cc)
target_link_libraries(power_json_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_power_json_test COMMAND power_json_test)

add_executable(star_sqlite_test star_sqlite_test.cc)
target_link_libraries(star_sqlite_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_star_sqlite_test COMMAND star_sqlite_test)

add_executable(planet_sqlite_test planet_sqlite_test.cc)
target_link_libraries(planet_sqlite_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_planet_sqlite_test COMMAND planet_sqlite_test)

add_executable(freeid_test freeid_test.cc)
target_link_libraries(freeid_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_freeid_test COMMAND freeid_test)

add_executable(numships_test numships_test.cc)
target_link_libraries(numships_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_numships_test COMMAND numships_test)

add_executable(database_test dal/database_test.cc)
target_link_libraries(database_test PRIVATE dallib)
add_test(NAME GB_database_test COMMAND database_test)

add_executable(json_store_test dal/json_store_test.cc)
target_link_libraries(json_store_test PRIVATE dallib)
add_test(NAME GB_json_store_test COMMAND json_store_test)

add_executable(race_repository_test repositories/race_repository_test.cc)
target_link_libraries(race_repository_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_race_repository_test COMMAND race_repository_test)

add_executable(ship_repository_test repositories/ship_repository_test.cc)
target_link_libraries(ship_repository_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_ship_repository_test COMMAND ship_repository_test)

add_executable(planet_repository_test repositories/planet_repository_test.cc)
target_link_libraries(planet_repository_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_planet_repository_test COMMAND planet_repository_test)

add_executable(star_repository_test repositories/star_repository_test.cc)
target_link_libraries(star_repository_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_star_repository_test COMMAND star_repository_test)

add_executable(sector_repository_test repositories/sector_repository_test.cc)
target_link_libraries(sector_repository_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_sector_repository_test COMMAND sector_repository_test)

add_executable(misc_repositories_test repositories/misc_repositories_test.cc)
target_link_libraries(misc_repositories_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_misc_repositories_test COMMAND misc_repositories_test)

add_executable(entity_manager_test services/entity_manager_test.cc)
target_link_libraries(entity_manager_test PRIVATE dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_entity_manager_test COMMAND entity_manager_test)

# Custom target to run GB with AddressSanitizer options for detailed leak detection
add_custom_target(run-gb-debug
    COMMAND ${CMAKE_COMMAND} -E env 
        "ASAN_OPTIONS=symbolize=1:detect_leaks=1:verbosity=1:log_to_syslog=0"
        $<TARGET_FILE:GB>
    DEPENDS GB
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/gb
    COMMENT "Running GB with AddressSanitizer options for detailed leak detection"
    USES_TERMINAL
)

# Only add docs if Doxygen is found
if(DOXYGEN_FOUND)
  doxygen_add_docs(gb-docs)
endif()
