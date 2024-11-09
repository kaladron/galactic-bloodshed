# SPDX-License-Identifier: Apache-2.0

file(GLOB docs *.doc)
install(FILES ${docs} DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}")