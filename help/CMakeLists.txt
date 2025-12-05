# SPDX-License-Identifier: Apache-2.0

file(GLOB help_files *.md)
install(FILES ${help_files} DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}/help")
