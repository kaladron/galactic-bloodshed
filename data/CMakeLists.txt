cmake_minimum_required(VERSION 2.8)
INSTALL(FILES exam.dat help.txt planet.list ship.dat star.list
  DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME})
