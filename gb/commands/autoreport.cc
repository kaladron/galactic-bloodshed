// SPDX-License-Identifier: Apache-2.0

/// \file autoreport.cc
/// \brief Auto-report toggling command.
/// \brief Tell server to generate a report for each planet.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool autoreport(const command_t& argv, GameObj& g) {
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
      auto place = Place::resolve(g.entity_manager, g.scope_context(), argv[1]);
      if (!place) {
        g.out << format_place_error(place.error());
        g.out << "Scope must be a planet.\n";
        return false;
      }
      if (place->level != ScopeLevel::LEVEL_PLAN) {
        g.out << "Scope must be a planet.\n";
        return false;
      }
      snum = place->snum;
      pnum = place->pnum;
    } break;
    default:
      g.out << "Invalid number of arguments.\n";
      return false;
  }

  const auto& target_star = *g.entity_manager.peek_star(snum);
  if (!target_star.control(g.player(), g.governor())) {
    g.out << "You are not authorized to do this here.\n";
    return false;
  }

  bool enabled = false;
  g.entity_manager.mutate_planet(snum, pnum, [&](Planet& p) {
    auto& info = p.info(g.player());
    enabled = (info.autorep == 0);
    info.autorep = enabled ? TELEG_MAX_AUTO : 0;
  });

  g.present(GB::presentation::AutoreportResult{
      .snum = snum,
      .pnum = pnum,
      .planet_name = target_star.get_planet_name(pnum),
      .enabled = enabled,
  });
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
