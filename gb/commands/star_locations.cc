// SPDX-License-Identifier: Apache-2.0

/// \file star_locations.cc
/// \brief Report stellar positions and distances from current position.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {
bool star_locations(const command_t& argv, GameObj& g) {
  if (argv.size() > 2) {
    g.out << "Syntax: stars [<radius>]\n";
    return false;
  }

  int max_dist = 999999;
  if (argv.size() == 2) {
    auto scan_res = scn::scan<int>(argv[1], "{}");
    if (!scan_res || !scan_res->range().empty() || scan_res->value() < 0) {
      g.out << "Bad distance value.\n";
      return false;
    }
    max_dist = scan_res->value();
  }

  GB::presentation::StarLocationsViewModel vm{.max_distance = max_dist};
  const auto center = g.universe_center();
  for (const Star& star : StarList::readonly(g.entity_manager)) {
    const double dist = star.coordinates().distance_to(center);
    if (std::floor(dist) <= max_dist) {
      vm.stars.push_back({
          .star_id = star.star_id(),
          .name = star.get_name(),
          .x = star.coordinates().x,
          .y = star.coordinates().y,
          .distance = dist,
      });
    }
  }

  g.present(vm);
  return true;
}

const CommandDescriptor stars_cmd{
    .name = "stars",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "stars [<radius>]",
    .description =
        "Report stellar positions and distances from current position",
    .handler = &star_locations,
};

}  // namespace GB::commands
