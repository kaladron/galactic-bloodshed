cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS '-DPKGDATADIR="${CMAKE_INSTALL_PREFIX}/data"')

add_definitions(-std=c++11)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions(-Werror -Weverything)

  # Slowly working on removing these
  add_definitions(-Wno-old-style-cast)
  add_definitions(-Wno-logical-not-parentheses)
  add_definitions(-Wno-conditional-uninitialized)
  add_definitions(-Wno-float-equal)
  add_definitions(-Wno-shadow)
  add_definitions(-Wno-unreachable-code)
  add_definitions(-Wno-unused-parameter)
  add_definitions(-Wno-sign-compare)
  add_definitions(-Wno-tautological-constant-out-of-range-compare) 

  add_definitions(-Wno-conversion)

  # These warnings seem like ones we'll never enable
  add_definitions(
    -Wno-switch-enum
    -Wno-disabled-macro-expansion
    -Wno-c++98-compat
    -Wno-padded)

  set(CMAKE_CXX_FLAGS_MSAN
    "-fsanitize=memory -fno-omit-frame-pointer -fno-optimize-sibling-calls"
    CACHE STRING "Flags used by the C++ compiler during MSan builds"
    FORCE)
  
  set(CMAKE_CXX_FLAGS_ASAN
    "-fsanitize=address"
    CACHE STRING "Flags used by the C++ compiler during ASan builds"
    FORCE)
  mark_as_advanced(
    CMAKE_CXX_FLAGS_ASAN
    CMAKE_CXX_FLAGS_MSAN)

  message("* Current build type is : ${CMAKE_BUILD_TYPE}")
ENDIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

add_executable(makeuniv
  files.cc
  files_rw.cc
  files_shl.cc
  makeuniv.cc
  makestar.cc
  makeplanet.cc
  perm.cc
  rand.cc)
target_link_libraries(makeuniv sqlite3) 

add_executable(GB
  GB_server.cc
  VN.cc
  analysis.cc
  autoreport.cc
  autoshoot.cc
  build.cc
  capital.cc
  capture.cc
  cs.cc
  declare.cc
  dissolve.cc
  dock.cc
  doplanet.cc
  dosector.cc
  doship.cc
  doturncmd.cc
  enslave.cc
  examine.cc
  explore.cc
  files.cc
  files_rw.cc
  files_shl.cc
  fire.cc
  fuel.cc
  get4args.cc
  getplace.cc
  land.cc
  launch.cc
  load.cc
  map.cc
  max.cc
  misc.cc
  mobiliz.cc
  move.cc
  moveplanet.cc
  moveship.cc
  name.cc
  orbit.cc
  order.cc
  perm.cc
  powercmd.cc
  prof.cc
  rand.cc
  relation.cc
  rst.cc
  scrap.cc
  shlmisc.cc
  shootblast.cc
  survey.cc
  tech.cc
  tele.cc
  toggle.cc
  toxicity.cc
  victory.cc
  zoom.cc)
target_link_libraries(GB sqlite3) 

add_executable(enrol
  enrol.cc
  files.cc
  files_rw.cc
  files_shl.cc
  max.cc
  perm.cc
  rand.cc
  shlmisc.cc)
target_link_libraries(enrol sqlite3 ncurses)

# new-fangled enrollment program (more flexible than 'enrol')
add_executable(racegen
  GB_racegen.cc
  enroll.cc
  files.cc
  files_rw.cc
  files_shl.cc
  max.cc
  perm.cc
  racegen.cc
  rand.cc
  shlmisc.cc)
target_link_libraries(racegen sqlite3)
set_target_properties(racegen PROPERTIES COMPILE_DEFINITIONS "PRIV -Wno-c++11-compat-deprecated-writable-strings")
