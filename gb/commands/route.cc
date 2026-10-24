// SPDX-License-Identifier: Apache-2.0

/// \file route.cc
/// \brief Set and view automated shipping routes for planets.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Parse and validate a 1-based route index (`1..MAX_ROUTES`).
 */
std::optional<int> parse_route_index(std::string_view arg, GameObj& g) {
  const auto parsed = scn::scan<int>(arg, "{}");
  if (!parsed || parsed->value() < 1 || parsed->value() > MAX_ROUTES) {
    g.out << "Bad route number.\n";
    return std::nullopt;
  }
  return parsed->value();
}

GB::presentation::RouteRow make_route_row(EntityManager& em, int route_num,
                                          const plroute& rt) {
  std::string star_name = "???";
  std::string planet_name = "???";
  if (rt.has_destination()) {
    const auto& dest_star = *em.peek_star(*rt.dest_star);
    star_name = dest_star.get_name();
    planet_name = dest_star.get_planet_name(*rt.dest_planet);
  }
  return GB::presentation::RouteRow{
      .route_num = route_num,
      .dest_coords = rt.dest_coords,
      .load = rt.load.format_compact(),
      .unload = rt.unload.format_compact(),
      .dest_star_name = std::move(star_name),
      .dest_planet_name = std::move(planet_name),
  };
}

GB::presentation::RouteSetResult make_route_set_result(int route_num,
                                                       const plroute& rt) {
  return GB::presentation::RouteSetResult{
      .route_num = route_num,
      .active = rt.set,
      .dest_coords = rt.dest_coords,
      .load = rt.load.format_compact(),
      .unload = rt.unload.format_compact(),
      .dest_star = rt.dest_star,
      .dest_planet = rt.dest_planet,
  };
}

/**
 * @brief Display all active shipping routes on the current planet.
 */
void show_all_routes(GameObj& g) {
  const player_t playernum = g.player();
  GB::presentation::RouteListViewModel vm;
  g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
    for (const int route_num : std::views::iota(1, MAX_ROUTES + 1)) {
      const auto& rt = p.info(playernum).route_at(route_num);
      if (rt.set) {
        vm.routes.push_back(make_route_row(g.entity_manager, route_num, rt));
      }
    }
  });
  g.present(vm);
}

/**
 * @brief Display a single shipping route by 1-based route number.
 */
void show_single_route(GameObj& g, int route_num) {
  const player_t playernum = g.player();
  GB::presentation::RouteListViewModel vm;
  g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
    const auto& rt = p.info(playernum).route_at(route_num);
    if (rt.set) {
      vm.routes.push_back(make_route_row(g.entity_manager, route_num, rt));
    }
  });
  g.present(vm);
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
    plroute updated{};
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      auto& rt = p.info(playernum).route_at(route_num);
      rt.set = active;
      updated = rt;
    });
    g.present(make_route_set_result(route_num, updated));
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
  plroute updated{};
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    auto& rt = p.info(playernum).route_at(route_num);
    rt.set_destination(where->snum, where->pnum);
    updated = rt;
  });
  g.present(make_route_set_result(route_num, updated));
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
    plroute updated{};
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      auto& rt = p.info(playernum).route_at(route_num);
      rt.dest_coords = *coords_opt;
      updated = rt;
    });
    g.present(make_route_set_result(route_num, updated));
    return true;
  }
  if (subcmd == "load") {
    plroute updated{};
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      auto& rt = p.info(playernum).route_at(route_num);
      rt.load = CommodityManifest::parse(value_arg);
      updated = rt;
    });
    g.present(make_route_set_result(route_num, updated));
    return true;
  }
  if (subcmd == "unload") {
    plroute updated{};
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      auto& rt = p.info(playernum).route_at(route_num);
      rt.unload = CommodityManifest::parse(value_arg);
      updated = rt;
    });
    g.present(make_route_set_result(route_num, updated));
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
