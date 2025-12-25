// SPDX-License-Identifier: Apache-2.0

/* scrap.c -- turn a ship to junk */

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void scrap(const command_t& argv, GameObj& g) {
  const ap_t APcount = 1;

  if (argv.size() < 2) {
    g.out << "Scrap what?\n";
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(g.governor, s)) continue;

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

    if (!enufAP(g.player, g.governor, star->AP(g.player - 1), APcount)) {
      continue;
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
          "{} is not landed or docked.\nNo resources can be reclaimed.\n",
          ship_to_string(s));
    }

    // Handle docked ship - use optional since EntityHandle has no default
    // constructor
    std::optional<EntityHandle<Ship>> s2_handle_opt;
    Ship* s2 = nullptr;
    if (docked(s)) {
      s2_handle_opt = g.entity_manager.get_ship(s.destshipno());
      if (!s2_handle_opt || !s2_handle_opt->get()) {
        continue;
      }
      s2 = &(*(*s2_handle_opt));
      // TODO(jeffbailey): Changed from !s.whatorbits, which didn't make any
      // sense.
      if ((!s2->docked() || s2->destshipno() != s.number()) &&
          s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
        g.out << "Warning, other ship not docked..\n";
        continue;
      }
    }

    int scrapval = shipcost(s) / 2 + s.resource();
    int destval = 0;
    int crewval = 0;
    int xtalval = 0;
    int troopval = 0;
    double fuelval = 0.0;

    // Get sector for landed ships on planets
    // Use optional since EntityHandle has no default constructor
    std::optional<EntityHandle<SectorMap>> smap_handle_opt;
    Sector* sect = nullptr;
    if (s.whatorbits() == ScopeLevel::LEVEL_PLAN && landed(s)) {
      smap_handle_opt =
          g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());
      if (smap_handle_opt && smap_handle_opt->get()) {
        sect = &smap_handle_opt->get()->get(s.land_x(), s.land_y());
      }
    }

    if (s.docked()) {
      g.out << std::format("{}: original cost: {}\n", ship_to_string(s),
                           shipcost(s));
      g.out << std::format("         scrap value{}: {} rp's.\n",
                           s.resource() ? "(with stockpile) " : "", scrapval);

      if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
          s2->resource() + scrapval > max_resource(*s2) &&
          s2->type() != ShipType::STYPE_SHUTTLE) {
        scrapval = max_resource(*s2) - s2->resource();
        g.out << std::format("(There is only room for {} resources.)\n",
                             scrapval);
      }

      if (s.fuel()) {
        g.out << std::format("Fuel recovery: {:.0f}.\n", s.fuel());
        fuelval = s.fuel();
        if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
            s2->fuel() + fuelval > max_fuel(*s2)) {
          fuelval = max_fuel(*s2) - s2->fuel();
          g.out << std::format("(There is only room for {:.2f} fuel.)\n",
                               fuelval);
        }
      } else {
        fuelval = 0.0;
      }

      if (s.destruct()) {
        g.out << std::format("Weapons recovery: {}.\n", s.destruct());
        destval = s.destruct();
        if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
            s2->destruct() + destval > max_destruct(*s2)) {
          destval = max_destruct(*s2) - s2->destruct();
          g.out << std::format("(There is only room for {} destruct.)\n",
                               destval);
        }
      } else {
        destval = 0;
      }

      if (s.popn() + s.troops()) {
        if (s.whatdest() == ScopeLevel::LEVEL_PLAN && sect != nullptr &&
            sect->get_owner() > 0 && sect->get_owner() != g.player) {
          g.out << "You don't own this sector; no crew can be recovered.\n";
        } else {
          g.out << std::format("Population/Troops recovery: {}/{}.\n", s.popn(),
                               s.troops());
          troopval = s.troops();
          if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
              s2->troops() + troopval > max_mil(*s2)) {
            troopval = max_mil(*s2) - s2->troops();
            g.out << std::format("(There is only room for {} troops.)\n",
                                 troopval);
          }
          crewval = s.popn();
          if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
              s2->popn() + crewval > max_crew(*s2)) {
            crewval = max_crew(*s2) - s2->popn();
            g.out << std::format("(There is only room for {} crew.)\n",
                                 crewval);
          }
        }
      } else {
        crewval = 0;
        troopval = 0;
      }

      if (s.crystals() + s.mounted()) {
        if (s.whatdest() == ScopeLevel::LEVEL_PLAN && sect != nullptr &&
            sect->get_owner() > 0 && sect->get_owner() != g.player) {
          g.out << "You don't own this sector; no crystals can be recovered.\n";
        } else {
          xtalval = s.crystals() + s.mounted();
          if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
              s2->crystals() + xtalval > max_crystals(*s2)) {
            xtalval = max_crystals(*s2) - s2->crystals();
            g.out << std::format("(There is only room for {} crystals.)\n",
                                 xtalval);
          }
          g.out << std::format("Crystal recovery: {}.\n", xtalval);
        }
      } else {
        xtalval = 0;
      }
    }

    /* more adjustments needed here for hanger. Maarten */
    if (s.whatorbits() == ScopeLevel::LEVEL_SHIP && s2 != nullptr) {
      s2->hanger() -= s.size();
    }

    if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
    }
    deductAPs(g, APcount, s.storbits());

    g.entity_manager.kill_ship(g.player, s);

    if (docked(s) && s2 != nullptr) {
      s2->crystals() += xtalval;
      rcv_fuel(*s2, fuelval);
      rcv_destruct(*s2, destval);
      rcv_resource(*s2, scrapval);
      rcv_troops(*s2, troopval, g.race->mass);
      rcv_popn(*s2, crewval, g.race->mass);
      /* check for docking status in case scrapped ship is landed. Maarten */
      if (s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
        s2->docked() = 0; /* undock the surviving ship */
        s2->whatdest() = ScopeLevel::LEVEL_UNIV;
        s2->destshipno() = 0;
      }
    }

    if (s.whatorbits() == ScopeLevel::LEVEL_PLAN) {
      auto planet_handle =
          g.entity_manager.get_planet(s.storbits(), s.pnumorbits());
      if (planet_handle.get() && landed(s) && sect != nullptr) {
        auto& planet = *planet_handle;
        if (sect->get_owner() == g.player) {
          sect->set_popn(sect->get_popn() + troopval);
          sect->set_popn(sect->get_popn() + crewval);
        } else if (sect->get_owner() == 0) {
          sect->set_owner(g.player);
          sect->set_popn(sect->get_popn() + crewval);
          sect->set_troops(sect->get_troops() + troopval);
          planet.info(g.player - 1).numsectsowned++;
          planet.info(g.player - 1).popn += crewval;
          planet.info(g.player - 1).popn += troopval;
          g.out << std::format("Sector {},{} Colonized.\n", s.land_x(),
                               s.land_y());
        }
        planet.info(g.player - 1).resource += scrapval;
        planet.popn() += crewval;
        planet.info(g.player - 1).destruct += destval;
        planet.info(g.player - 1).fuel += static_cast<int>(fuelval);
        planet.info(g.player - 1).crystals += xtalval;
      }
    }

    if (landed(s)) {
      g.out << "\nScrapped.\n";
    } else {
      g.out << "\nDestroyed.\n";
    }
  }
}
}  // namespace GB::commands
