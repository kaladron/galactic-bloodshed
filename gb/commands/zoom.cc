// SPDX-License-Identifier: Apache-2.0

/// \file zoom.cc
/// \brief Functions for implementing the 'zoom' command.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;
#undef stdout

module commands;

namespace GB::commands {
/// Zoom in or out for orbit display
bool zoom(const command_t& argv, GameObj& g) {
  int i = (g.level() == ScopeLevel::LEVEL_UNIV);

  if (argv.size() > 1) {
    auto scan_res = scn::scan<double, double>(argv[1], "{}/{}");
    if (scan_res) {
      auto [num, denom] = scan_res->values();
      if (denom == 0.0) {
        g.out << "Illegal denominator value.\n";
        return false;
      }
      g.zoom[i] = num / denom;
    } else {
      auto single_res = scn::scan<double>(argv[1], "{}");
      if (single_res) {
        g.zoom[i] = single_res->value();
      }
    }
  }

  const auto [lastx, lasty] =
      (g.level() == ScopeLevel::LEVEL_UNIV)
          ? std::pair{g.universe_center().x, g.universe_center().y}
          : std::pair{g.system_center().x, g.system_center().y};

  g.out << std::format("Zoom value {0}, lastx = {1}, lasty = {2}.\n", g.zoom[i],
                       lastx, lasty);
  return true;
}

const CommandDescriptor zoom_cmd{
    .name = "zoom",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "zoom [<amount>]",
    .description = "Set zoom scale and center coordinates for orbit display",
    .handler = &zoom,
};

}  // namespace GB::commands
