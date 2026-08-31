# SPDX-License-Identifier: Apache-2.0

# Install non-help documentation files
install(FILES combat.md covert_ops.md diplomacy.md economy.md gb_FAQ.txt
              geoengineering.md governance.md navigation.md
              planetary_simulation.md planets.md RACEGEN.COMPILE.HELP
              RACEGEN.PLAYER.HELP races.md ships.md ship_types.md stars.md
              turn_cycle.md von_neumann.md
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")