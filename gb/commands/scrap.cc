// SPDX-License-Identifier: Apache-2.0

/// \file scrap.cc
/// \brief Scrap ships for raw materials.

module;

import std;
import gblib;

module commands;

namespace GB::commands {

bool scrap(const command_t& argv, GameObj& g) {
  bool any_scrapped = false;

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(g.governor(), s)) continue;

    if (s.max_crew() && !s.popn()) {
      g.out << "Can't scrap that ship - no crew.\n";
      continue;
    }
    if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      continue;
    }

    const auto* star = g.entity_manager.peek_star(s.storbits());
    if (!star) {
      g.out << "Star not found.\n";
      continue;
    }

    if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      if (!g.deduct_univ_ap(1)) {
        g.out << "You need 1 universe action point.\n";
        continue;
      }
    } else {
      if (!g.deduct_ap(s.storbits(), 1)) {
        g.out << "You don't have 1 action points there.\n";
        continue;
      }
    }

    if (s.whatorbits() == ScopeLevel::LEVEL_PLAN &&
        s.type() == ShipType::OTYPE_TOXWC) {
      std::string toxin_amount = "0";
      if (std::holds_alternative<WasteData>(s.special())) {
        auto waste = std::get<WasteData>(s.special());
        toxin_amount = std::to_string(waste.toxic);
      }
      g.out << std::format("WARNING: This will release {} toxin points back "
                           "into the atmosphere!!\n",
                           toxin_amount);
    }

    if (!s.docked()) {
      g.out << std::format(
          "{} is not landed or docked.\nNo resources can be reclaimed.\n", s);
    }

    if (docked(s)) {
      bool valid_dock = true;
      try {
        g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
          if ((!s2.docked() || s2.destshipno() != s.number()) &&
              s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
            g.out << "Warning, other ship not docked..\n";
            valid_dock = false;
          }
        });
      } catch (const EntityNotFoundError&) {
        continue;
      }
      if (!valid_dock) {
        continue;
      }
    }

    int scrapval = shipcost(s) / 2 + s.resource();
    int destval = 0;
    int crewval = 0;
    int xtalval = 0;
    int troopval = 0;
    double fuelval = 0.0;

    // Check sector owner for landed ships on planets
    player_t sect_owner = 0;
    bool is_landed_on_planet =
        (s.whatorbits() == ScopeLevel::LEVEL_PLAN && landed(s));
    if (is_landed_on_planet) {
      g.entity_manager.with_sectormap(
          s.storbits(), s.pnumorbits(), [&](const SectorMap& smap) {
            sect_owner = smap.get(s.land_coords()).get_owner();
          });
    }

    if (s.docked()) {
      g.out << std::format("{}: original cost: {}\n", s, shipcost(s));
      g.out << std::format("         scrap value{}: {} rp's.\n",
                           s.resource() ? "(with stockpile) " : "", scrapval);

      if (s.fuel()) {
        fuelval = s.fuel();
      } else {
        fuelval = 0.0;
      }

      if (s.destruct()) {
        destval = s.destruct();
      } else {
        destval = 0;
      }

      if (s.popn() + s.troops()) {
        if (s.whatdest() == ScopeLevel::LEVEL_PLAN && is_landed_on_planet &&
            sect_owner > 0 && sect_owner != g.player()) {
          g.out << "You don't own this sector; no crew can be recovered.\n";
        } else {
          troopval = s.troops();
          crewval = s.popn();
        }
      } else {
        crewval = 0;
        troopval = 0;
      }

      if (s.crystals() + s.mounted()) {
        if (s.whatdest() == ScopeLevel::LEVEL_PLAN && is_landed_on_planet &&
            sect_owner > 0 && sect_owner != g.player()) {
          g.out << "You don't own this sector; no crystals can be recovered.\n";
        } else {
          xtalval = s.crystals() + s.mounted();
        }
      } else {
        xtalval = 0;
      }

      if (s.whatdest() == ScopeLevel::LEVEL_SHIP) {
        g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
          if (s2.resource() + scrapval > max_resource(s2) &&
              s2.type() != ShipType::STYPE_SHUTTLE) {
            scrapval = max_resource(s2) - s2.resource();
            g.out << std::format("(There is only room for {} resources.)\n",
                                 scrapval);
          }

          if (s.fuel()) {
            g.out << std::format("Fuel recovery: {:.0f}.\n", s.fuel());
            if (s2.fuel() + fuelval > max_fuel(s2)) {
              fuelval = max_fuel(s2) - s2.fuel();
              g.out << std::format("(There is only room for {:.2f} fuel.)\n",
                                   fuelval);
            }
          }

          if (s.destruct()) {
            g.out << std::format("Weapons recovery: {}.\n", s.destruct());
            if (s2.destruct() + destval > max_destruct(s2)) {
              destval = max_destruct(s2) - s2.destruct();
              g.out << std::format("(There is only room for {} destruct.)\n",
                                   destval);
            }
          }

          if (s.popn() + s.troops() &&
              !(is_landed_on_planet && sect_owner > 0 &&
                sect_owner != g.player())) {
            g.out << std::format("Population/Troops recovery: {}/{}.\n",
                                 s.popn(), s.troops());
            if (s2.troops() + troopval > max_mil(s2)) {
              troopval = max_mil(s2) - s2.troops();
              g.out << std::format("(There is only room for {} troops.)\n",
                                   troopval);
            }
            if (s2.popn() + crewval > max_crew(s2)) {
              crewval = max_crew(s2) - s2.popn();
              g.out << std::format("(There is only room for {} crew.)\n",
                                   crewval);
            }
          }

          if (s.crystals() + s.mounted() &&
              !(is_landed_on_planet && sect_owner > 0 &&
                sect_owner != g.player())) {
            if (s2.crystals() + xtalval > max_crystals(s2)) {
              xtalval = max_crystals(s2) - s2.crystals();
              g.out << std::format("(There is only room for {} crystals.)\n",
                                   xtalval);
            }
            g.out << std::format("Crystal recovery: {}.\n", xtalval);
          }
        });
      } else {
        if (s.fuel()) {
          g.out << std::format("Fuel recovery: {:.0f}.\n", s.fuel());
        }
        if (s.destruct()) {
          g.out << std::format("Weapons recovery: {}.\n", s.destruct());
        }
        if (s.popn() + s.troops() && !(is_landed_on_planet && sect_owner > 0 &&
                                       sect_owner != g.player())) {
          g.out << std::format("Population/Troops recovery: {}/{}.\n", s.popn(),
                               s.troops());
        }
        if (s.crystals() + s.mounted() &&
            !(is_landed_on_planet && sect_owner > 0 &&
              sect_owner != g.player())) {
          g.out << std::format("Crystal recovery: {}.\n", xtalval);
        }
      }
    }

    /* more adjustments needed here for hanger. Maarten */
    if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      g.entity_manager.mutate_ship(s.destshipno(),
                                   [&](Ship& s2) { s2.hanger() -= s.size(); });
    }

    g.entity_manager.kill_ship(g.player(), s);

    if (docked(s)) {
      g.entity_manager.mutate_ship(s.destshipno(), [&](Ship& s2) {
        s2.crystals() += xtalval;
        rcv_fuel(s2, fuelval);
        rcv_destruct(s2, destval);
        rcv_resource(s2, scrapval);
        rcv_troops(s2, troopval, g.race->mass);
        rcv_popn(s2, crewval, g.race->mass);
        /* check for docking status in case scrapped ship is landed. Maarten */
        if (s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
          s2.docked() = 0; /* undock the surviving ship */
          s2.whatdest() = ScopeLevel::LEVEL_UNIV;
          s2.destshipno() = 0;
        }
      });
    }

    if (is_landed_on_planet) {
      g.entity_manager.mutate_sectormap(
          s.storbits(), s.pnumorbits(), [&](SectorMap& smap) {
            g.entity_manager.mutate_planet(
                s.storbits(), s.pnumorbits(), [&](Planet& planet) {
                  auto& sector = smap.get(s.land_coords());
                  if (sector.get_owner() == g.player()) {
                    sector.add_popn(troopval);
                    sector.add_popn(crewval);
                  } else if (sector.get_owner() == 0) {
                    sector.set_owner(g.player());
                    sector.add_popn(crewval);
                    sector.set_troops(sector.get_troops() + troopval);
                    planet.info(g.player()).numsectsowned++;
                    planet.info(g.player()).popn += crewval;
                    planet.info(g.player()).popn += troopval;
                    g.out << std::format("Sector {} Colonized.\n",
                                         s.land_coords());
                  }
                  planet.info(g.player()).resource += scrapval;
                  planet.popn() += crewval;
                  planet.info(g.player()).destruct += destval;
                  planet.info(g.player()).fuel += static_cast<int>(fuelval);
                  planet.info(g.player()).crystals += xtalval;
                });
          });
    }

    if (landed(s)) {
      g.out << "\nScrapped.\n";
    } else {
      g.out << "\nDestroyed.\n";
    }
    any_scrapped = true;
  }

  return any_scrapped;
}

const CommandDescriptor scrap_cmd{
    .name = "scrap",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "scrap <ship>",
    .description = "Scrap a ship to reclaim resources, fuel, and crew",
    .handler = &scrap,
};

}  // namespace GB::commands
