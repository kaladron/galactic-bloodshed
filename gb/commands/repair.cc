// SPDX-License-Identifier: Apache-2.0

/// \file repair.cc
/// \brief Repair ships and structures.

module;

import std;
import gb.entities;
import gb.services;

module commands;

namespace GB::commands {
bool repair(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  int hix;
  int lowy;
  int hiy;
  int x2;

  std::unique_ptr<Place> where;
  if (argv.size() == 1) { /* no args */
    where = std::make_unique<Place>(g.level(), g.snum(), g.pnum());
  } else {
    /* repairing a sector */
    if (std::isdigit(argv[1][0]) && argv[1].find(',') != std::string::npos) {
      if (g.level() != ScopeLevel::LEVEL_PLAN) {
        g.out << "There are no sectors here.\n";
        return false;
      }
      where =
          std::make_unique<Place>(ScopeLevel::LEVEL_PLAN, g.snum(), g.pnum());

    } else {
      where = std::make_unique<Place>(g, argv[1]);
      if (where->err || where->level == ScopeLevel::LEVEL_SHIP) return false;
    }
  }

  if (where->level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Scope must be a planet.\n";
    return false;
  }

  bool valid_planet = g.entity_manager.with_planet(
      where->snum, where->pnum, [&](const Planet& p) {
        if (!p.info(Playernum).numsectsowned) {
          g.out << "You don't own any sectors on this planet.\n";
          return false;
        }

        if (argv.size() > 1 && !argv[1].empty() && std::isdigit(argv[1][0]) &&
            argv[1].find(',') != std::string::npos) {
          // translate from lowx:hix,lowy:hiy
          auto coords = get4args(argv[1]);
          if (!coords) {
            g.out << "Invalid coordinate format. Use: x,y or xl:xh,yl:yh\n";
            return false;
          }
          auto [x_low, x_high, y_low, y_high] = *coords;
          x2 = std::max(0, x_low);
          hix = std::min(x_high, p.dimensions().x - 1);
          lowy = std::max(0, y_low);
          hiy = std::min(y_high, p.dimensions().y - 1);
        } else {
          /* repair entire planet */
          x2 = 0;
          hix = p.dimensions().x - 1;
          lowy = 0;
          hiy = p.dimensions().y - 1;
        }
        return true;
      });
  if (!valid_planet) return false;

  int sectors = 0;
  int cost = 0;
  g.entity_manager.mutate_sectormap(
      where->snum, where->pnum, [&](SectorMap& smap) {
        g.entity_manager.mutate_planet(
            where->snum, where->pnum, [&](Planet& p) {
              for (int y = lowy; y <= hiy; y++) {
                for (int lowx = x2; lowx <= hix; lowx++) {
                  if (p.info(Playernum).resource >= SECTOR_REPAIR_COST) {
                    auto& s = smap.get(Coordinates{lowx, y});
                    if (s.is_wasted() &&
                        (s.get_owner() == Playernum || !s.is_owned())) {
                      s.set_condition(s.get_type());
                      s.set_fert(std::min(100U, s.get_fert() + 20));
                      p.info(Playernum).resource -= SECTOR_REPAIR_COST;
                      cost += SECTOR_REPAIR_COST;
                      sectors += 1;
                    }
                  }
                }
              }
            });
      });

  g.out << std::format("{0} sectors repaired at a cost of {1} resources.\n",
                       sectors, cost);
  return true;
}

const CommandDescriptor repair_cmd{
    .name = "repair",
    .roles = {},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "repair [<coords>]",
    .description = "Repair wasted sectors on a planet",
    .handler = &repair,
};

}  // namespace GB::commands
