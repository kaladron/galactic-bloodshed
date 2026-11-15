# SPDX-License-Identifier: Apache-2.0

file(GLOB help_files CONFIGURE_DEPENDS *.md)
install(FILES ${help_files} DESTINATION "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/help")
