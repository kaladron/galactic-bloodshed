cmake_minimum_required(VERSION 2.8)

file(GLOB docs *.doc)
install(FILES ${docs} DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}")
