// SPDX-License-Identifier: Apache-2.0

/// \file autoreport.cc
/// \brief Auto-report toggling command.
/// \brief Tell server to generate a report for each planet.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool autoreport(const command_t& argv, GameObj& g) {
  bool authorized = g.entity_manager.with_star(g.snum(), [&](const Star& star) {
    return (g.governor() == 0 || star.governor(g.player()) == g.governor());
  });

  if (!authorized) {
    g.out << "You are not authorized to do this here.\n";
    return false;
  }

  starnum_t snum = 0;
  planetnum_t pnum = 0;

  switch (argv.size()) {
    case 1:
      if (g.level() != ScopeLevel::LEVEL_PLAN) {
        g.out << "Scope must be a planet.\n";
        return false;
      }
      snum = g.snum();
      pnum = g.pnum();
      break;
    case 2: {
      Place place{g, argv[1]};
      if (place.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "Scope must be a planet.\n";
        return false;
      }
      snum = place.snum;
      pnum = place.pnum;
    } break;
    default:
      g.out << "Invalid number of arguments.\n";
      return false;
  }

  bool is_set = false;
  g.entity_manager.mutate_planet(snum, pnum, [&](Planet& p) {
    if (p.info(g.player()).autorep) {
      p.info(g.player()).autorep = 0;
      is_set = false;
    } else {
      p.info(g.player()).autorep = TELEG_MAX_AUTO;
      is_set = true;
    }
  });

  std::string planet_name;
  g.entity_manager.with_star(snum, [&](const Star& target_star) {
    planet_name = target_star.get_planet_name(pnum);
  });
  g.out << std::format("Autoreport on {0} has been {1}.\n",
                       planet_name.empty() ? "Unknown" : planet_name,
                       is_set ? "set" : "unset");
  return true;
}

const CommandDescriptor autoreport_cmd{
    .name = "autoreport",
    .roles = {.star_control = true},
    .scopes = AllowedScopes::non_universe(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "autoreport [<planet>]",
    .description = "Toggle automatic production reports for a planet",
    .handler = &autoreport,
};

}  // namespace GB::commands
