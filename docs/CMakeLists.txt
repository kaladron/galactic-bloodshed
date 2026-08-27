# SPDX-License-Identifier: Apache-2.0

# Install non-help documentation files
install(FILES economy.md gb_FAQ.txt governance.md planetary_simulation.md
              planets.md RACEGEN.COMPILE.HELP RACEGEN.PLAYER.HELP
        DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}")