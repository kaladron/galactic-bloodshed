// SPDX-License-Identifier: Apache-2.0

/// \file route.cc
/// \brief Set and view automated shipping routes for planets.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace {

/**
 * @brief Parse and validate a 1-based route index (`1..MAX_ROUTES`).
 */
std::optional<int> parse_route_index(std::string_view arg, GameObj& g) {
  const auto parsed = parse_int<int>(arg);
  if (!parsed || *parsed < 1 || *parsed > MAX_ROUTES) {
    g.out << "Bad route number.\n";
    return std::nullopt;
  }
  return *parsed;
}

/**
 * @brief Display all active shipping routes on the current planet.
 */
void show_all_routes(GameObj& g) {
  g.present(GB::presentation::RouteListViewModel{
      .planet = GB::presentation::project_planet_view(
          g.entity_manager, g.scope_context(), g.snum(), g.pnum())});
}

/**
 * @brief Display a single shipping route by 1-based route number.
 */
void show_single_route(GameObj& g, int route_num) {
  g.present(GB::presentation::RouteListViewModel{
      .planet = GB::presentation::project_planet_view(
          g.entity_manager, g.scope_context(), g.snum(), g.pnum()),
      .route_num = route_num});
}

/**
 * @brief Handle 3-argument route commands (`activate`, `deactivate`, or
 * `<destination>`).
 */
bool configure_route_target(GameObj& g, int route_num,
                            const std::string& target_arg) {
  const player_t playernum = g.player();
  if (target_arg == "activate" || target_arg == "deactivate") {
    const bool active = (target_arg == "activate");
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      p.info(playernum).route_at(route_num).set = active;
    });
    g.present(GB::presentation::make_output_message("Set.\n"));
    return true;
  }

  const auto where =
      Place::resolve(g.entity_manager, g.scope_context(), target_arg, true);
  if (!where) {
    g.out << format_place_error(where.error());
    g.out << "Illegal destination.\n";
    return false;
  }
  if (where->level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to designate a planet.\n";
    return false;
  }
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    p.info(playernum).route_at(route_num).set_destination(where->snum,
                                                          where->pnum);
  });
  g.present(GB::presentation::make_output_message("Set.\n"));
  return true;
}

/**
 * @brief Handle 4-argument route commands (`land`, `load`, `unload`).
 */
bool configure_route_attribute(GameObj& g, int route_num,
                               std::string_view subcmd,
                               std::string_view value_arg) {
  const player_t playernum = g.player();
  if (subcmd == "land") {
    const auto coords_opt = Coordinates::parse(value_arg);
    const auto* planet = g.entity_manager.peek_planet(g.snum(), g.pnum());
    if (!coords_opt || !planet->is_valid(*coords_opt)) {
      g.out << "Bad sector coordinates.\n";
      return false;
    }
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      p.info(playernum).route_at(route_num).dest_coords = *coords_opt;
    });
    g.present(GB::presentation::make_output_message("Set.\n"));
    return true;
  }
  if (subcmd == "load") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      p.info(playernum).route_at(route_num).load =
          CommodityManifest::parse(value_arg);
    });
    g.present(GB::presentation::make_output_message("Set.\n"));
    return true;
  }
  if (subcmd == "unload") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      p.info(playernum).route_at(route_num).unload =
          CommodityManifest::parse(value_arg);
    });
    g.present(GB::presentation::make_output_message("Set.\n"));
    return true;
  }

  g.out << "What are you trying to do?\n";
  return false;
}

}  // namespace

namespace GB::commands {

bool route(const command_t& argv, GameObj& g) {
  if (argv.size() == 1) {
    show_all_routes(g);
    return true;
  }

  const auto route_num = parse_route_index(argv[1], g);
  if (!route_num) {
    return false;
  }

  if (argv.size() == 2) {
    show_single_route(g, *route_num);
    return true;
  }
  if (argv.size() == 3) {
    return configure_route_target(g, *route_num, argv[2]);
  }
  if (argv.size() == 4) {
    return configure_route_attribute(g, *route_num, argv[2], argv[3]);
  }
  g.out << "What are you trying to do?\n";
  return false;
}

const CommandDescriptor route_cmd{
    .name = "route",
    .roles = {},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "route [<number> [activate|deactivate|land|load|unload|<dest> "
              "[<args>]]]",
    .description = "Set and view automated shipping routes for a planet",
    .handler = &route,
};

}  // namespace GB::commands
