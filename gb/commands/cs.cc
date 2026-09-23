// SPDX-License-Identifier: Apache-2.0

/// \file cs.cc
/// \brief Change current scope level or default home system.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace {

/**
 * @brief Reset the governor's current scope to their configured default home
 * scope.
 */
bool change_to_default_scope(GameObj& g) {
  const starnum_t numstars = g.entity_manager.num_stars();
  if (numstars == 0) {
    g.out << "cs: Universe data not available.\n";
    return false;
  }

  const auto& gov = g.race->governor[g.governor().value];
  g.set_level(gov.deflevel);
  g.set_snum(gov.defsystem);
  if (g.snum() < 1 || g.snum() > numstars) {
    g.set_snum(numstars);
  }

  const auto& star = *g.entity_manager.peek_star(g.snum());
  g.set_pnum(gov.defplanetnum);
  if (g.pnum() < 1 || g.pnum() > star.numplanets()) {
    g.set_pnum(star.numplanets());
  }

  g.set_shipno(0);
  g.set_system_center({0.0, 0.0});
  g.set_universe_center(star.coordinates());
  return true;
}

/**
 * @brief Update viewport center coordinates when navigating away from a planet
 * scope.
 */
void update_viewport_from_planet(GameObj& g, const Place& where) {
  const auto& planet = *g.entity_manager.peek_planet(g.snum(), g.pnum());
  if (where.level == ScopeLevel::LEVEL_STAR && where.snum == g.snum()) {
    g.set_system_center(planet.system_coordinates());
  } else if (where.level == ScopeLevel::LEVEL_UNIV) {
    const auto& star = *g.entity_manager.peek_star(g.snum());
    g.set_universe_center(planet.absolute_coordinates(star));
  } else {
    g.set_system_center({0.0, 0.0});
  }
}

/**
 * @brief Update viewport center coordinates when navigating away from a ship
 * scope.
 */
void update_viewport_from_ship(GameObj& g, const Place& where) {
  const auto* s = g.entity_manager.peek_ship(g.shipno());
  if (!s || s->docked()) {
    g.set_system_center({0.0, 0.0});
    return;
  }

  if (where.level == ScopeLevel::LEVEL_UNIV) {
    g.set_universe_center(s->coordinates());
    return;
  }
  if (where.level == ScopeLevel::LEVEL_STAR &&
      s->whatorbits() >= ScopeLevel::LEVEL_STAR &&
      s->storbits() == where.snum) {
    const auto& orbit_star = *g.entity_manager.peek_star(s->storbits());
    g.set_system_center(s->coordinates() - orbit_star.coordinates());
    return;
  }
  if (where.level == ScopeLevel::LEVEL_PLAN &&
      s->whatorbits() == ScopeLevel::LEVEL_PLAN &&
      s->storbits() == where.snum && s->pnumorbits() == where.pnum) {
    const auto& planet =
        *g.entity_manager.peek_planet(s->storbits(), s->pnumorbits());
    const auto& orbit_star = *g.entity_manager.peek_star(s->storbits());
    g.set_system_center(s->coordinates() -
                        planet.absolute_coordinates(orbit_star));
    return;
  }
  g.set_system_center({0.0, 0.0});
}

/**
 * @brief Update viewport center coordinates based on the origin and destination
 * scopes.
 */
void update_viewport_center(GameObj& g, const Place& where) {
  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      g.set_system_center({0.0, 0.0});
      break;
    case ScopeLevel::LEVEL_STAR:
      if (where.level == ScopeLevel::LEVEL_UNIV) {
        const auto& star = *g.entity_manager.peek_star(g.snum());
        g.set_universe_center(star.coordinates());
      } else {
        g.set_system_center({0.0, 0.0});
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      update_viewport_from_planet(g, where);
      break;
    case ScopeLevel::LEVEL_SHIP:
      update_viewport_from_ship(g, where);
      break;
  }
}

/**
 * @brief Update the governor's persistent default home scope (`cs -d <scope>`).
 */
bool change_default_home_scope(GameObj& g, std::string_view target_arg) {
  Place where{g, target_arg};
  if (where.err || where.level == ScopeLevel::LEVEL_SHIP) {
    g.out << "cs: bad home system.\n";
    return false;
  }

  const player_t playernum = g.player();
  const governor_t governor = g.governor();
  g.entity_manager.mutate_race(playernum, [&](Race& race) {
    race.governor[governor.value].deflevel = where.level;
    race.governor[governor.value].defsystem = where.snum;
    race.governor[governor.value].defplanetnum = where.pnum;
  });

  g.out << std::format("New home system is {}\n", where.to_string());
  return true;
}

}  // namespace

namespace GB::commands {

bool cs(const command_t& argv, GameObj& g) {
  if (argv.size() == 1) {
    return change_to_default_scope(g);
  }

  if (argv.size() == 2) {
    Place where{g, argv[1]};
    if (where.err) {
      g.out << "cs: bad scope.\n";
      g.set_system_center({0.0, 0.0});
      return false;
    }

    update_viewport_center(g, where);
    g.set_level(where.level);
    g.set_snum(where.snum);
    g.set_pnum(where.pnum);
    g.set_shipno(where.shipno);
    return true;
  }

  if (argv.size() == 3 && argv[1] == "-d") {
    return change_default_home_scope(g, argv[2]);
  }

  g.out << "cs: bad scope.\n";
  return false;
}

const CommandDescriptor cs_cmd{
    .name = "cs",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "cs [<scope> | -d <scope>]",
    .description = "Change current scope level or default home system",
    .handler = &cs,
};

}  // namespace GB::commands
