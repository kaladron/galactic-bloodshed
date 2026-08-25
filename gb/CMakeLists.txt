# SPDX-License-Identifier: Apache-2.0

# Treat warnings as errors for all targets in gb/ directory
add_compile_options(-Werror)

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

# Make Doxygen optional - gracefully skip documentation if not found
find_package(Doxygen QUIET OPTIONAL_COMPONENTS dot)
if(NOT DOXYGEN_FOUND)
  message(STATUS "Doxygen not found - documentation generation will be skipped")
endif()

# Include directories
include_directories(..)

# Attempt to find SQLite3 on the system
find_package(SQLite3 REQUIRED)

# Libraries

# Create test utilities module (standalone - not part of gblib)
add_library(test_module)
target_sources(
  test_module
  PUBLIC FILE_SET CXX_MODULES FILES test.cppm)
target_link_libraries(test_module PUBLIC gblib dallib session commands)

# Executable targets
add_executable(makeuniv creator/makeplanet.cc creator/makestar.cc
                        creator/makeuniv.cc)
target_link_libraries(makeuniv PRIVATE gblib SQLite::SQLite3 glaze::glaze)

add_executable(enrol enrol.cc)
target_link_libraries(enrol PRIVATE gblib SQLite::SQLite3 glaze::glaze scnlib_module)

add_executable(racegen GB_racegen.cc enroll.cc racegen.cc)
target_link_libraries(racegen PRIVATE gblib dallib SQLite::SQLite3 glaze::glaze)

# Create types module as a standalone foundational library
# This contains basic type definitions used across all layers
add_library(types_module)
target_sources(
  types_module
  PUBLIC FILE_SET CXX_MODULES FILES types.cppm)
target_link_libraries(types_module PUBLIC strong_id_module)

# Create dallib as a separate library for Data Access Layer
add_library(dallib)
target_sources(
  dallib
  PUBLIC FILE_SET CXX_MODULES FILES dal/dallib.cppm
  PRIVATE dal/database.cc dal/json_store.cc dal/schema.cc)
target_link_libraries(dallib PUBLIC SQLite::SQLite3 types_module)

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
         gblib-prompt.cppm
         gblib-race.cppm
         utils/gblib-rand.cppm
         repositories/gblib-repositories.cppm
         services/gblib-services.cppm
         gblib-sector.cppm
         gblib-sessionregistry.cppm
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
          gameobj.cc
          load.cc
          map.cc
          misc.cc
          move.cc
          order.cc
          planet.cc
          prompt.cc
          utils/rand.cc
          sectormap.cc
          sector.cc
          ships.cc
          entitylists.cc
          shlmisc.cc
          shootblast.cc
          star.cc
          tele.cc
          victory.cc
          VN.cc)
target_link_libraries(gblib PUBLIC glaze::glaze glaze_json_module glaze_core_module strong_id_module dallib types_module)

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

# Create auth library (Service Layer)
add_library(auth)
target_sources(
  auth
  PUBLIC FILE_SET CXX_MODULES FILES services/auth.cppm
  PRIVATE services/auth.cc)
target_link_libraries(auth PUBLIC gblib session commands)

# Create server_config library (Service Layer)
add_library(server_config)
target_sources(
  server_config
  PUBLIC FILE_SET CXX_MODULES FILES services/server_config.cppm
  PRIVATE services/server_config.cc)
target_link_libraries(server_config PUBLIC gblib dallib)

# Create server library (Service Layer)
add_library(server)
target_sources(
  server
  PUBLIC FILE_SET CXX_MODULES FILES services/server.cppm
  PRIVATE services/server.cc)
target_link_libraries(server PUBLIC gblib asio_module session auth notification commands dallib)

add_library(commands)
target_sources(
  commands
  PUBLIC FILE_SET CXX_MODULES FILES commands/commands.cppm
                                    commands/command_spec.cppm
  PRIVATE commands/allocate.cc
          commands/analysis.cc
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
          commands/dispatch.cc
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
          commands/registry.cc
          commands/relation.cc
          commands/repair.cc
          commands/route.cc
          commands/rst.cc
          commands/segment.cc
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
          commands/time.cc
          commands/toggle.cc
          commands/toxicity.cc
          commands/transfer.cc
          commands/treasury.cc
          commands/unpledge.cc
          commands/update.cc
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
target_link_libraries(GB PRIVATE gblib commands asio_module session auth server_config server SQLite::SQLite3 glaze::glaze)
install(TARGETS GB)

# Test helper function
function(gb_add_test name)
  cmake_parse_arguments(ARG "" "" "SOURCES;EXTRA_LIBS" ${ARGN})
  if(NOT ARG_SOURCES)
    set(ARG_SOURCES ${name}.cc)
  endif()
  add_executable(${name} ${ARG_SOURCES})
  target_link_libraries(${name} PRIVATE
    test_module dallib gblib commands SQLite::SQLite3 glaze::glaze ${ARG_EXTRA_LIBS}
  )
  add_test(NAME GB_${name} COMMAND ${name})
endfunction()

# Test targets
gb_add_test(asio_test EXTRA_LIBS asio_module)
gb_add_test(scnlib_test EXTRA_LIBS scnlib_module)

gb_add_test(allocate_test SOURCES commands/allocate_test.cc)
gb_add_test(analysis_test SOURCES commands/analysis_test.cc)
gb_add_test(announce_test SOURCES commands/announce_test.cc EXTRA_LIBS session notification)
gb_add_test(arm_test SOURCES commands/arm_test.cc)
gb_add_test(auth_test SOURCES services/auth_test.cc EXTRA_LIBS session auth)
gb_add_test(autoreport_test SOURCES commands/autoreport_test.cc)
gb_add_test(berserker_bombard_test)
gb_add_test(bid_test SOURCES commands/bid_test.cc)
gb_add_test(bitops_test)
gb_add_test(bless_test SOURCES commands/bless_test.cc)
gb_add_test(block_json_test)
gb_add_test(block_test SOURCES commands/block_test.cc)
gb_add_test(bombard_test SOURCES commands/bombard_test.cc)
gb_add_test(build_command_test SOURCES commands/build_test.cc)
gb_add_test(build_table_test SOURCES commands/build_table_test.cc)
gb_add_test(build_test)
gb_add_test(build_test_extended SOURCES commands/build_test_extended.cc)
gb_add_test(capital_test SOURCES commands/capital_test.cc)
gb_add_test(capture_test SOURCES commands/capture_test.cc)
gb_add_test(center_test SOURCES commands/center_test.cc)
gb_add_test(colonies_test SOURCES commands/colonies_test.cc)
gb_add_test(command_registry_test SOURCES commands/command_registry_test.cc)
gb_add_test(commod_json_test)
gb_add_test(coordinates_test)
gb_add_test(cs_test SOURCES commands/cs_test.cc)
gb_add_test(database_test SOURCES dal/database_test.cc)
gb_add_test(declare_test SOURCES commands/declare_test.cc)
gb_add_test(defend_test SOURCES commands/defend_test.cc)
gb_add_test(detonate_test SOURCES commands/detonate_test.cc)
gb_add_test(dispatch_pipeline_test SOURCES commands/dispatch_pipeline_test.cc)
gb_add_test(dissolve_test SOURCES commands/dissolve_test.cc)
gb_add_test(distance_test SOURCES commands/distance_test.cc)
gb_add_test(dock_test SOURCES commands/dock_test.cc)
gb_add_test(doplanet_test)
gb_add_test(dosector_simple_test)
gb_add_test(doship_repair_test)
gb_add_test(doship_test)
gb_add_test(doturn_test)
gb_add_test(dump_test SOURCES commands/dump_test.cc)
gb_add_test(emulate_test SOURCES commands/emulate_test.cc)
gb_add_test(enroll_test SOURCES enroll_test.cc GB_racegen.cc EXTRA_LIBS scnlib_module)
gb_add_test(enslave_test SOURCES commands/enslave_test.cc)
gb_add_test(entity_lists_test)
gb_add_test(entity_manager_kill_ship_test SOURCES services/entity_manager_kill_ship_test.cc)
gb_add_test(entity_manager_test SOURCES services/entity_manager_test.cc)
gb_add_test(estimate_test)
gb_add_test(examine_test SOURCES commands/examine_test.cc)
gb_add_test(explore_test SOURCES commands/explore_test.cc)
gb_add_test(fire_test SOURCES commands/fire_test.cc)
gb_add_test(fix_test SOURCES commands/fix_test.cc)
gb_add_test(freeid_test)
gb_add_test(fuel_test SOURCES commands/fuel_test.cc)
gb_add_test(gameobj_ap_test)
gb_add_test(gblib_test)
gb_add_test(give_test SOURCES commands/give_test.cc)
gb_add_test(governors_test SOURCES commands/governors_test.cc)
gb_add_test(grant_test SOURCES commands/grant_test.cc)
gb_add_test(ground_attack_test)
gb_add_test(help_command_test SOURCES commands/help_test.cc)
gb_add_test(help_test)
gb_add_test(highlight_test SOURCES commands/highlight_test.cc)
gb_add_test(insurgency_test SOURCES commands/insurgency_test.cc)
gb_add_test(invariant_log_test)
gb_add_test(invite_test SOURCES commands/invite_test.cc)
gb_add_test(jettison_test SOURCES commands/jettison_test.cc)
gb_add_test(json_store_test SOURCES dal/json_store_test.cc)
gb_add_test(land_test SOURCES commands/land_test.cc)
gb_add_test(launch_test SOURCES commands/launch_test.cc)
gb_add_test(load_test SOURCES commands/load_test.cc)
gb_add_test(make_mod_test SOURCES commands/make_mod_test.cc)
gb_add_test(makeplanet_test SOURCES creator/makeplanet_test.cc creator/makeplanet.cc creator/makestar.cc)
gb_add_test(map_test SOURCES commands/map_test.cc)
gb_add_test(misc_repositories_test SOURCES repositories/misc_repositories_test.cc)
gb_add_test(mobilize_test SOURCES commands/mobilize_test.cc)
gb_add_test(motto_test SOURCES commands/motto_test.cc)
gb_add_test(mount_test SOURCES commands/mount_test.cc)
gb_add_test(move_popn_test SOURCES commands/move_popn_test.cc)
gb_add_test(move_test)
gb_add_test(name_test SOURCES commands/name_test.cc)
gb_add_test(news_test)
gb_add_test(notification_test EXTRA_LIBS session notification)
gb_add_test(numships_test)
gb_add_test(orbit_test SOURCES commands/orbit_test.cc)
gb_add_test(order_test SOURCES commands/order_test.cc)
gb_add_test(page_test SOURCES commands/page_test.cc EXTRA_LIBS session notification)
gb_add_test(pay_test SOURCES commands/pay_test.cc)
gb_add_test(personal_test SOURCES commands/personal_test.cc)
gb_add_test(planet_repository_test SOURCES repositories/planet_repository_test.cc)
gb_add_test(planet_sqlite_test)
gb_add_test(planet_test)
gb_add_test(pledge_test SOURCES commands/pledge_test.cc)
gb_add_test(power_command_test SOURCES commands/power_test.cc EXTRA_LIBS tabulate::tabulate)
gb_add_test(power_json_test)
gb_add_test(production_test SOURCES commands/production_test.cc)
gb_add_test(profile_test SOURCES commands/profile_test.cc)
gb_add_test(prompt_test)
gb_add_test(purge_test SOURCES commands/purge_test.cc)
gb_add_test(quit_test SOURCES commands/quit_test.cc)
gb_add_test(race_repository_test SOURCES repositories/race_repository_test.cc)
gb_add_test(relation_test SOURCES commands/relation_test.cc)
gb_add_test(repair_test SOURCES commands/repair_test.cc)
gb_add_test(route_test SOURCES commands/route_test.cc)
gb_add_test(rst_test SOURCES commands/rst_test.cc)
gb_add_test(schedule_test SOURCES commands/schedule_test.cc)
gb_add_test(scrap_test SOURCES commands/scrap_test.cc)
gb_add_test(sector_repository_test SOURCES repositories/sector_repository_test.cc)
gb_add_test(sector_test)
gb_add_test(sectormap_sqlite_test)
gb_add_test(segment_test SOURCES commands/segment_test.cc)
gb_add_test(sell_test SOURCES commands/sell_test.cc)
gb_add_test(send_message_test SOURCES commands/send_message_test.cc)
gb_add_test(server_command_test EXTRA_LIBS session)
gb_add_test(server_config_test SOURCES services/server_config_test.cc EXTRA_LIBS server_config)
gb_add_test(server_test EXTRA_LIBS asio_module session auth server notification)
gb_add_test(session_test EXTRA_LIBS session)
gb_add_test(ship_repository_test SOURCES repositories/ship_repository_test.cc)
gb_add_test(shipfilter_test)
gb_add_test(shiplist_test)
gb_add_test(shlmisc_test)
gb_add_test(shootblast_test)
gb_add_test(shutdown_test SOURCES commands/shutdown_test.cc)
gb_add_test(star_locations_test SOURCES commands/star_locations_test.cc)
gb_add_test(star_repository_test SOURCES repositories/star_repository_test.cc)
gb_add_test(star_sqlite_test)
gb_add_test(star_test)
gb_add_test(stardata_json_test)
gb_add_test(strong_id_json_test EXTRA_LIBS glaze_core_module)
gb_add_test(strong_id_test EXTRA_LIBS strong_id_module)
gb_add_test(survey_test SOURCES commands/survey_test.cc)
gb_add_test(tactical_test SOURCES commands/tactical_test.cc EXTRA_LIBS tabulate::tabulate)
gb_add_test(tax_test SOURCES commands/tax_test.cc)
gb_add_test(tech_status_test SOURCES commands/tech_status_test.cc)
gb_add_test(technology_test SOURCES commands/technology_test.cc)
gb_add_test(telegram_test)
gb_add_test(test_context_test)
gb_add_test(time_test SOURCES commands/time_test.cc)
gb_add_test(toggle_test SOURCES commands/toggle_test.cc)
gb_add_test(toxicity_test SOURCES commands/toxicity_test.cc)
gb_add_test(transfer_test SOURCES commands/transfer_test.cc)
gb_add_test(treasury_test SOURCES commands/treasury_test.cc)
gb_add_test(universe_test)
gb_add_test(unpledge_test SOURCES commands/unpledge_test.cc)
gb_add_test(update_test SOURCES commands/update_test.cc)
gb_add_test(upgrade_test SOURCES commands/upgrade_test.cc)
gb_add_test(victory_test SOURCES commands/victory_test.cc EXTRA_LIBS scnlib_module)
gb_add_test(vn_steal_test)
gb_add_test(vote_test SOURCES commands/vote_test.cc EXTRA_LIBS session notification)
gb_add_test(walk_test SOURCES commands/walk_test.cc)
gb_add_test(who_test SOURCES commands/who_test.cc)
gb_add_test(whois_test SOURCES commands/whois_test.cc)
gb_add_test(zoom_test SOURCES commands/zoom_test.cc)

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
