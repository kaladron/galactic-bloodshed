// SPDX-License-Identifier: Apache-2.0

/// \file repair.cc
/// \brief Repair ships and structures.

module commands;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

namespace GB::commands {
bool repair(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();

  Place where{g.level(), g.snum(), g.pnum()};
  bool has_coords = false;
  if (argv.size() > 1) {
    if (std::isdigit(static_cast<unsigned char>(argv[1][0])) &&
        argv[1].contains(',')) {
      has_coords = true;
    } else {
      auto resolved =
          Place::resolve(g.entity_manager, g.scope_context(), argv[1]);
      if (!resolved) {
        g.out << format_place_error(resolved.error());
        return false;
      }
      if (resolved->level != ScopeLevel::LEVEL_PLAN) {
        g.out << "Scope must be a planet.\n";
        return false;
      }
      where = *resolved;
    }
  }

  const auto& planet_ref =
      *g.entity_manager.peek_planet(where.snum, where.pnum);
  if (!planet_ref.info(Playernum).numsectsowned) {
    g.out << "You don't own any sectors on this planet.\n";
    return false;
  }

  std::pair<Coordinates, Coordinates> bounds{
      Coordinates{0, 0}, planet_ref.dimensions() - Coordinates{1, 1}};
  if (has_coords) {
    auto coords = get4args(argv[1]);
    if (!coords) {
      g.out << "Invalid coordinate format. Use: x,y or xl:xh,yl:yh\n";
      return false;
    }
    bounds = *coords;
  }

  const auto [low, high] = bounds;
  int sectors = 0;
  resource_t cost = 0;
  g.entity_manager.mutate_sectormap(
      where.snum, where.pnum, [&](SectorMap& smap) {
        g.entity_manager.mutate_planet(where.snum, where.pnum, [&](Planet& p) {
          for (auto [c, s] : smap.indexed_sectors()) {
            if (!c.within_bounds(low, high)) continue;
            if (p.info(Playernum).resource >= SECTOR_REPAIR_COST &&
                s.is_repairable_by(Playernum)) {
              s.repair();
              p.info(Playernum).resource -= SECTOR_REPAIR_COST;
              cost += SECTOR_REPAIR_COST;
              sectors += 1;
            }
          }
        });
      });

  g.present(GB::presentation::RepairResult{
      .sectors_repaired = sectors,
      .resource_cost = cost,
  });
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
