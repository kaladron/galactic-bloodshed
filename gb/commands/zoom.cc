// SPDX-License-Identifier: Apache-2.0

/// \file zoom.cc
/// \brief Functions for implementing the 'zoom' command.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

bool parse_zoom_factor(std::string_view arg, double& out_zoom, GameObj& g) {
  if (arg.contains('/')) {
    auto scan_res = scn::scan<double, double>(arg, "{}/{}");
    if (!scan_res || !scan_res->range().empty()) {
      g.out << "Bad zoom value.\n";
      return false;
    }
    const auto [num, denom] = scan_res->values();
    if (denom == 0.0) {
      g.out << "Illegal denominator value.\n";
      return false;
    }
    out_zoom = num / denom;
    return true;
  }

  auto single_res = scn::scan<double>(arg, "{}");
  if (!single_res || !single_res->range().empty()) {
    g.out << "Bad zoom value.\n";
    return false;
  }
  out_zoom = single_res->value();
  return true;
}

}  // namespace

namespace GB::commands {
/// Zoom in or out for orbit display
bool zoom(const command_t& argv, GameObj& g) {
  if (argv.size() > 2) {
    g.out << "Syntax: zoom [<amount>]\n";
    return false;
  }

  const int i = (g.level() == ScopeLevel::LEVEL_UNIV) ? 1 : 0;
  if (argv.size() == 2 && !parse_zoom_factor(argv[1], g.zoom[i], g)) {
    return false;
  }

  const double last_x = (g.level() == ScopeLevel::LEVEL_UNIV)
                            ? g.universe_center().x
                            : g.system_center().x;
  const double last_y = (g.level() == ScopeLevel::LEVEL_UNIV)
                            ? g.universe_center().y
                            : g.system_center().y;
  g.present(GB::presentation::ZoomResult{
      .zoom = g.zoom[i],
      .last_x = last_x,
      .last_y = last_y,
  });
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
