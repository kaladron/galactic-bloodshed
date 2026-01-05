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
    # Development mode: use source help directory
    set(HELPDIR
        "${CMAKE_SOURCE_DIR}/help/"
        CACHE STRING "Path to the help files directory")
    message(STATUS "Debug build: Using source help directory: ${HELPDIR}")
else()
    # Production mode: use install directory
    set(DOCDIR
        "${CMAKE_INSTALL_FULL_DOCDIR}/"
        CACHE STRING "Path to the package document directory")
    message(STATUS "Release build: Using install docs directory: ${DOCDIR}")
    # Production mode: use install directory for help files
    set(HELPDIR
        "${CMAKE_INSTALL_FULL_DOCDIR}/help/"
        CACHE STRING "Path to the help files directory")
    message(STATUS "Release build: Using install help directory: ${HELPDIR}")
endif()
add_definitions(-DDOCDIR="${DOCDIR}")
add_definitions(-DHELPDIR="${HELPDIR}")

# Require dot, treat the other components as optional
find_package(Doxygen OPTIONAL_COMPONENTS dot)

# Bring in
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

# Include directories
include_directories(..)

# Attempt to find SQLite3 on the system
find_package(SQLite3 REQUIRED)

# Libraries

# Create tabulate module wrapper library
add_library(tabulate_module)
target_sources(
  tabulate_module
  PUBLIC FILE_SET CXX_MODULES FILES third_party/tabulate.cppm)
target_link_libraries(tabulate_module PUBLIC tabulate::tabulate)

# Create scnlib module wrapper library
add_library(scnlib_module)
target_sources(
  scnlib_module
  PUBLIC FILE_SET CXX_MODULES FILES third_party/scnlib.cppm)
target_link_libraries(scnlib_module PUBLIC scn::scn)

# Create glaze JSON module wrapper library
add_library(glaze_json_module)
target_sources(
  glaze_json_module
  PUBLIC FILE_SET CXX_MODULES FILES third_party/glaze_json.cppm)
target_link_libraries(glaze_json_module PUBLIC glaze::glaze)

# Create glaze TOML module wrapper library
add_library(glaze_toml_module)
target_sources(
  glaze_toml_module
  PUBLIC FILE_SET CXX_MODULES FILES third_party/glaze_toml.cppm)
target_link_libraries(glaze_toml_module PUBLIC glaze::glaze)

# Create asio_standalone interface library
add_library(asio_standalone INTERFACE)
target_link_libraries(asio_standalone INTERFACE Boost::headers)
target_include_directories(asio_standalone INTERFACE 
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
  PUBLIC FILE_SET CXX_MODULES FILES third_party/asio.cppm)
target_link_libraries(asio_module PUBLIC asio_standalone)

# Create test utilities module (standalone - not part of gblib)
add_library(test_module)
target_sources(
  test_module
  PUBLIC FILE_SET CXX_MODULES FILES test.cppm)
target_link_libraries(test_module PUBLIC gblib dallib session)

# Executable targets
add_executable(makeuniv creator/makeplanet.cc creator/makestar.cc
                        creator/makeuniv.cc)
target_link_libraries(makeuniv PRIVATE gblib SQLite::SQLite3 glaze::glaze)

add_executable(enrol enrol.cc)
target_link_libraries(enrol PRIVATE gblib SQLite::SQLite3 glaze::glaze scnlib_module)

add_executable(racegen GB_racegen.cc enroll.cc racegen.cc)
target_link_libraries(racegen PRIVATE gblib dallib SQLite::SQLite3 glaze::glaze)

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
         gblib-csp.cppm
         gblib-doplanet.cppm
         gblib-dosector.cppm
         gblib-doship.cppm
         gblib-doturncmd.cppm
         gblib-entitylists.cppm
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
         gblib-shipfilter.cppm
         gblib-shlmisc.cppm
         gblib-shootblast.cppm
         gblib-star.cppm
         gblib-tele.cppm
         gblib-turnstats.cppm
         gblib-tweakables.cppm
         gblib-types.cppm
         gblib-universe.cppm
  PRIVATE bombard.cc
          build.cc
          doplanet.cc
          dosector.cc
          doship.cc
          doturncmd.cc
          entitylists.cc
          services/entity_manager.cc
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
          entitylists.cc
          shlmisc.cc
          shootblast.cc
          star.cc
          tele.cc
          victory.cc
          VN.cc)
target_link_libraries(gblib PUBLIC glaze::glaze glaze_json_module dallib)

# Create session library (Service Layer)
add_library(session)
target_sources(
  session
  PUBLIC FILE_SET CXX_MODULES FILES services/session.cppm
  PRIVATE services/session.cc)
target_link_libraries(session PUBLIC gblib asio_module)

# Create notification library (Service Layer)
add_library(notification)
target_sources(
  notification
  PUBLIC FILE_SET CXX_MODULES FILES services/notification.cppm
  PRIVATE services/notification.cc)
target_link_libraries(notification PUBLIC gblib session)

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
          commands/emulate.cc
          commands/enslave.cc
          commands/examine.cc
          commands/explore.cc
          commands/fire.cc
          commands/fix.cc
          commands/give.cc
          commands/governors.cc
          commands/grant.cc
          commands/help.cc
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
          commands/purge.cc
          commands/quit.cc
          commands/read_messages.cc
          commands/relation.cc
          commands/repair.cc
          commands/route.cc
          commands/rst.cc
          commands/tactical.cc
          commands/scrap.cc
          commands/sell.cc
          commands/send_message.cc
          commands/shutdown.cc
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
          commands/who.cc
          commands/whois.cc
          commands/zoom.cc)
target_link_libraries(commands PRIVATE gblib session notification SQLite::SQLite3 glaze::glaze
                                       scnlib_module tabulate_module)

add_executable(GB GB_server.cc)
target_link_libraries(GB PRIVATE gblib commands asio_module session SQLite::SQLite3 glaze::glaze)
install(TARGETS GB)

# Test targets
add_executable(scnlib_test scnlib_test.cc)
target_link_libraries(scnlib_test PRIVATE scnlib_module)
add_test(NAME GB_scnlib_test COMMAND scnlib_test)

add_executable(asio_test asio_test.cc)
target_link_libraries(asio_test PRIVATE asio_module)
add_test(NAME GB_asio_test COMMAND asio_test)

add_executable(session_test session_test.cc)
target_link_libraries(session_test PRIVATE gblib session)
add_test(NAME GB_session_test COMMAND session_test)

add_executable(server_test server_test.cc)
target_link_libraries(server_test PRIVATE gblib session asio_module dallib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_server_test COMMAND server_test)

add_executable(notification_test notification_test.cc)
target_link_libraries(notification_test PRIVATE gblib session notification dallib)
add_test(NAME GB_notification_test COMMAND notification_test)

add_executable(who_test who_test.cc)
target_link_libraries(who_test PRIVATE gblib dallib)
add_test(NAME GB_who_test COMMAND who_test)

add_executable(emulate_test emulate_test.cc)
target_link_libraries(emulate_test PRIVATE gblib dallib)
add_test(NAME GB_emulate_test COMMAND emulate_test)

add_executable(shlmisc_test shlmisc_test.cc)
target_link_libraries(shlmisc_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_shlmisc_test COMMAND shlmisc_test)

add_executable(shiplist_test shiplist_test.cc)
target_link_libraries(shiplist_test PRIVATE test_module gblib dallib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_shiplist_test COMMAND shiplist_test)

add_executable(shipfilter_test shipfilter_test.cc)
target_link_libraries(shipfilter_test PRIVATE gblib dallib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_shipfilter_test COMMAND shipfilter_test)

add_executable(gblib_test gblib_test.cc)
target_link_libraries(gblib_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_gblib_test COMMAND gblib_test)

add_executable(invariant_log_test invariant_log_test.cc)
target_link_libraries(invariant_log_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_invariant_log_test COMMAND invariant_log_test)

add_executable(estimate_test estimate_test.cc)
target_link_libraries(estimate_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_estimate_test COMMAND estimate_test)

add_executable(build_test build_test.cc)
target_link_libraries(build_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_build_test COMMAND build_test)

add_executable(move_test move_test.cc)
target_link_libraries(move_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_move_test COMMAND move_test)

add_executable(dosector_simple_test dosector_simple_test.cc)
target_link_libraries(dosector_simple_test PRIVATE gblib SQLite::SQLite3)
add_test(NAME GB_dosector_simple_test COMMAND dosector_simple_test)

add_executable(sectormap_sqlite_test sectormap_sqlite_test.cc)
target_link_libraries(sectormap_sqlite_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_sectormap_sqlite_test COMMAND sectormap_sqlite_test)

add_executable(stardata_json_test stardata_json_test.cc)
target_link_libraries(stardata_json_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_stardata_json_test COMMAND stardata_json_test)

add_executable(commod_json_test commod_json_test.cc)
target_link_libraries(commod_json_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_commod_json_test COMMAND commod_json_test)

add_executable(block_json_test block_json_test.cc)
target_link_libraries(block_json_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_block_json_test COMMAND block_json_test)

add_executable(power_json_test power_json_test.cc)
target_link_libraries(power_json_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_power_json_test COMMAND power_json_test)

add_executable(star_test star_test.cc)
target_link_libraries(star_test PRIVATE gblib)
add_test(NAME GB_star_test COMMAND star_test)

add_executable(star_sqlite_test star_sqlite_test.cc)
target_link_libraries(star_sqlite_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_star_sqlite_test COMMAND star_sqlite_test)

add_executable(planet_sqlite_test planet_sqlite_test.cc)
target_link_libraries(planet_sqlite_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_planet_sqlite_test COMMAND planet_sqlite_test)

add_executable(freeid_test freeid_test.cc)
target_link_libraries(freeid_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_freeid_test COMMAND freeid_test)

add_executable(numships_test numships_test.cc)
target_link_libraries(numships_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_numships_test COMMAND numships_test)

add_executable(database_test dal/database_test.cc)
target_link_libraries(database_test PRIVATE dallib)
add_test(NAME GB_database_test COMMAND database_test)

add_executable(json_store_test dal/json_store_test.cc)
target_link_libraries(json_store_test PRIVATE dallib)
add_test(NAME GB_json_store_test COMMAND json_store_test)

add_executable(race_repository_test repositories/race_repository_test.cc)
target_link_libraries(race_repository_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_race_repository_test COMMAND race_repository_test)

add_executable(ship_repository_test repositories/ship_repository_test.cc)
target_link_libraries(ship_repository_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_ship_repository_test COMMAND ship_repository_test)

add_executable(planet_repository_test repositories/planet_repository_test.cc)
target_link_libraries(planet_repository_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_planet_repository_test COMMAND planet_repository_test)

add_executable(star_repository_test repositories/star_repository_test.cc)
target_link_libraries(star_repository_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_star_repository_test COMMAND star_repository_test)

add_executable(sector_repository_test repositories/sector_repository_test.cc)
target_link_libraries(sector_repository_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_sector_repository_test COMMAND sector_repository_test)

add_executable(misc_repositories_test repositories/misc_repositories_test.cc)
target_link_libraries(misc_repositories_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_misc_repositories_test COMMAND misc_repositories_test)

add_executable(entity_manager_test services/entity_manager_test.cc)
target_link_libraries(entity_manager_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_entity_manager_test COMMAND entity_manager_test)

add_executable(entity_manager_kill_ship_test services/entity_manager_kill_ship_test.cc)
target_link_libraries(entity_manager_kill_ship_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_entity_manager_kill_ship_test COMMAND entity_manager_kill_ship_test)

add_executable(entity_lists_test entity_lists_test.cc)
target_link_libraries(entity_lists_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_entity_lists_test COMMAND entity_lists_test)

add_executable(universe_test universe_test.cc)
target_link_libraries(universe_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_universe_test COMMAND universe_test)

add_executable(autoreport_test commands/autoreport_test.cc)
target_link_libraries(autoreport_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_autoreport_test COMMAND autoreport_test)

add_executable(tactical_test commands/tactical_test.cc)
target_link_libraries(tactical_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze tabulate::tabulate)
add_test(NAME GB_tactical_test COMMAND tactical_test)

add_executable(motto_test commands/motto_test.cc)
target_link_libraries(motto_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_motto_test COMMAND motto_test)

add_executable(highlight_test commands/highlight_test.cc)
target_link_libraries(highlight_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_highlight_test COMMAND highlight_test)

add_executable(capital_test commands/capital_test.cc)
target_link_libraries(capital_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_capital_test COMMAND capital_test)

add_executable(bless_test commands/bless_test.cc)
target_link_libraries(bless_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_bless_test COMMAND bless_test)

add_executable(tax_test commands/tax_test.cc)
target_link_libraries(tax_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_tax_test COMMAND tax_test)

add_executable(technology_test commands/technology_test.cc)
target_link_libraries(technology_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_technology_test COMMAND technology_test)

add_executable(mobilize_test commands/mobilize_test.cc)
target_link_libraries(mobilize_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_mobilize_test COMMAND mobilize_test)

add_executable(name_test commands/name_test.cc)
target_link_libraries(name_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_name_test COMMAND name_test)

add_executable(declare_test commands/declare_test.cc)
target_link_libraries(declare_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_declare_test COMMAND declare_test)

add_executable(grant_test commands/grant_test.cc)
target_link_libraries(grant_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_grant_test COMMAND grant_test)

add_executable(dissolve_test commands/dissolve_test.cc)
target_link_libraries(dissolve_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_dissolve_test COMMAND dissolve_test)

add_executable(route_test commands/route_test.cc)
target_link_libraries(route_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_route_test COMMAND route_test)

add_executable(toxicity_test commands/toxicity_test.cc)
target_link_libraries(toxicity_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_toxicity_test COMMAND toxicity_test)

add_executable(toggle_test commands/toggle_test.cc)
target_link_libraries(toggle_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_toggle_test COMMAND toggle_test)

add_executable(pay_test commands/pay_test.cc)
target_link_libraries(pay_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_pay_test COMMAND pay_test)

add_executable(order_test commands/order_test.cc)
target_link_libraries(order_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_order_test COMMAND order_test)

add_executable(cs_test commands/cs_test.cc)
target_link_libraries(cs_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_cs_test COMMAND cs_test)

add_executable(orbit_test commands/orbit_test.cc)
target_link_libraries(orbit_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_orbit_test COMMAND orbit_test)

add_executable(dock_test commands/dock_test.cc)
target_link_libraries(dock_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_dock_test COMMAND dock_test)

add_executable(dump_test commands/dump_test.cc)
target_link_libraries(dump_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_dump_test COMMAND dump_test)

add_executable(jettison_test commands/jettison_test.cc)
target_link_libraries(jettison_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_jettison_test COMMAND jettison_test)

add_executable(scrap_test commands/scrap_test.cc)
target_link_libraries(scrap_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_scrap_test COMMAND scrap_test)

add_executable(sell_test commands/sell_test.cc)
target_link_libraries(sell_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_sell_test COMMAND sell_test)

add_executable(transfer_test commands/transfer_test.cc)
target_link_libraries(transfer_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_transfer_test COMMAND transfer_test)

add_executable(bid_test commands/bid_test.cc)
target_link_libraries(bid_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_bid_test COMMAND bid_test)

add_executable(survey_test commands/survey_test.cc)
target_link_libraries(survey_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_survey_test COMMAND survey_test)

add_executable(upgrade_test commands/upgrade_test.cc)
target_link_libraries(upgrade_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_upgrade_test COMMAND upgrade_test)

add_executable(make_mod_test commands/make_mod_test.cc)
target_link_libraries(make_mod_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_make_mod_test COMMAND make_mod_test)

add_executable(map_test commands/map_test.cc)
target_link_libraries(map_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_map_test COMMAND map_test)

add_executable(mount_test commands/mount_test.cc)
target_link_libraries(mount_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_mount_test COMMAND mount_test)

add_executable(fix_test commands/fix_test.cc)
target_link_libraries(fix_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_fix_test COMMAND fix_test)

# Batch 10 tests (sector operations)
add_executable(arm_test commands/arm_test.cc)
target_link_libraries(arm_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_arm_test COMMAND arm_test)

add_executable(defend_test commands/defend_test.cc)
target_link_libraries(defend_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_defend_test COMMAND defend_test)

add_executable(repair_test commands/repair_test.cc)
target_link_libraries(repair_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_repair_test COMMAND repair_test)

# Batch 11 tests (population/ground operations)
add_executable(move_popn_test commands/move_popn_test.cc)
target_link_libraries(move_popn_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_move_popn_test COMMAND move_popn_test)

add_executable(walk_test commands/walk_test.cc)
target_link_libraries(walk_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_walk_test COMMAND walk_test)

add_executable(enslave_test commands/enslave_test.cc)
target_link_libraries(enslave_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_enslave_test COMMAND enslave_test)

add_executable(insurgency_test commands/insurgency_test.cc)
target_link_libraries(insurgency_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_insurgency_test COMMAND insurgency_test)

add_executable(land_test commands/land_test.cc)
target_link_libraries(land_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_land_test COMMAND land_test)

add_executable(launch_test commands/launch_test.cc)
target_link_libraries(launch_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_launch_test COMMAND launch_test)

add_executable(build_command_test commands/build_test.cc)
target_link_libraries(build_command_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_build_command_test COMMAND build_command_test)

add_executable(build_test_extended commands/build_test_extended.cc)
target_link_libraries(build_test_extended PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_build_test_extended COMMAND build_test_extended)

add_executable(build_table_test commands/build_table_test.cc)
target_link_libraries(build_table_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_build_table_test COMMAND build_table_test)

add_executable(capture_test commands/capture_test.cc)
target_link_libraries(capture_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_capture_test COMMAND capture_test)

add_executable(detonate_test commands/detonate_test.cc)
target_link_libraries(detonate_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_detonate_test COMMAND detonate_test)

add_executable(fire_test commands/fire_test.cc)
target_link_libraries(fire_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_fire_test COMMAND fire_test)

add_executable(bombard_test commands/bombard_test.cc)
target_link_libraries(bombard_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_bombard_test COMMAND bombard_test)

add_executable(give_test commands/give_test.cc)
target_link_libraries(give_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_give_test COMMAND give_test)

add_executable(help_test help_test.cc)
target_link_libraries(help_test PRIVATE gblib)
add_test(NAME GB_help_test COMMAND help_test)

add_executable(quit_test commands/quit_test.cc)
target_link_libraries(quit_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_quit_test COMMAND quit_test)

add_executable(purge_test commands/purge_test.cc)
target_link_libraries(purge_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_purge_test COMMAND purge_test)

add_executable(shutdown_test commands/shutdown_test.cc)
target_link_libraries(shutdown_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_shutdown_test COMMAND shutdown_test)

add_executable(load_test commands/load_test.cc)
target_link_libraries(load_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_load_test COMMAND load_test)

add_executable(send_message_test commands/send_message_test.cc)
target_link_libraries(send_message_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_send_message_test COMMAND send_message_test)

add_executable(news_test news_test.cc)
target_link_libraries(news_test PRIVATE test_module dallib gblib SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_news_test COMMAND news_test)

add_executable(analysis_test commands/analysis_test.cc)
target_link_libraries(analysis_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_analysis_test COMMAND analysis_test)

add_executable(block_test commands/block_test.cc)
target_link_libraries(block_test PRIVATE test_module dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME GB_block_test COMMAND block_test)

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
