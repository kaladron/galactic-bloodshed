// SPDX-License-Identifier: Apache-2.0

/// \file route.cc
/// \brief Set and view automated shipping routes for planets.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;
import tabulate;
#undef stdout

module commands;

namespace {

/**
 * @brief Parse and validate a 1-based route index (`1..MAX_ROUTES`).
 */
std::optional<int> parse_route_index(std::string_view arg, GameObj& g) {
  auto parsed = scn::scan<int>(arg, "{}");
  if (!parsed || parsed->value() < 1 || parsed->value() > MAX_ROUTES) {
    g.out << "Bad route number.\n";
    return std::nullopt;
  }
  return parsed->value();
}

tabulate::Table create_routes_table() {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.column(0).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(11);
  table.column(2).format().width(12);
  table.column(3).format().width(14);
  table.column(4).format().width(24);
  return table;
}

void append_route_row(tabulate::Table& table, EntityManager& em, int route_num,
                      const plroute& rt) {
  const auto* dest_star = em.peek_star(rt.dest_star);
  const std::string star_name = dest_star ? dest_star->get_name() : "???";
  const std::string planet_name =
      (dest_star && rt.dest_planet >= 1 &&
       rt.dest_planet <= dest_star->numplanets())
          ? dest_star->get_planet_name(rt.dest_planet)
          : "???";

  table.add_row({
      std::format("{}", route_num),
      std::format("land {:2},{:2}", rt.dest_coords.x, rt.dest_coords.y),
      rt.load.any() ? std::format("load: {}", rt.load.format_compact()) : "",
      rt.unload.any() ? std::format("unload: {}", rt.unload.format_compact())
                      : "",
      std::format("-> {}/{}", star_name, planet_name),
  });
}

/**
 * @brief Display all active shipping routes on the current planet.
 */
void show_all_routes(GameObj& g) {
  const player_t playernum = g.player();
  tabulate::Table table = create_routes_table();
  g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
    int route_num = 0;
    for (const auto& rt : p.info(playernum).route) {
      ++route_num;
      if (rt.set) {
        append_route_row(table, g.entity_manager, route_num, rt);
      }
    }
  });
  if (table.size() > 0) {
    g.out << table << "\n";
  }
  g.out << "Done.\n";
}

/**
 * @brief Display a single shipping route by 1-based route number.
 */
void show_single_route(GameObj& g, int route_num) {
  const player_t playernum = g.player();
  tabulate::Table table = create_routes_table();
  g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
    const auto& rt = p.info(playernum).route_at(route_num);
    if (rt.set) {
      append_route_row(table, g.entity_manager, route_num, rt);
    }
  });
  if (table.size() > 0) {
    g.out << table << "\n";
  }
  g.out << "Done.\n";
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
    g.out << "Set.\n";
    return true;
  }

  Place where{g, target_arg, true};
  if (where.err) {
    g.out << "Illegal destination.\n";
    return false;
  }
  if (where.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to designate a planet.\n";
    return false;
  }
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    auto& rt = p.info(playernum).route_at(route_num);
    rt.dest_star = where.snum;
    rt.dest_planet = where.pnum;
  });
  g.out << "Set.\n";
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
    auto coords_opt = Coordinates::parse(value_arg);
    if (!coords_opt) {
      g.out << "Bad sector coordinates.\n";
      return false;
    }
    const Coordinates coords = *coords_opt;
    bool valid = false;
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      if (!p.is_valid(coords)) {
        return;
      }
      valid = true;
      p.info(playernum).route_at(route_num).dest_coords = coords;
    });
    if (!valid) {
      g.out << "Bad sector coordinates.\n";
      return false;
    }
    g.out << "Set.\n";
    return true;
  }
  if (subcmd == "load") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      p.info(playernum).route_at(route_num).load =
          CommodityManifest::parse(value_arg);
    });
    g.out << "Set.\n";
    return true;
  }
  if (subcmd == "unload") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      p.info(playernum).route_at(route_num).unload =
          CommodityManifest::parse(value_arg);
    });
    g.out << "Set.\n";
    return true;
  }

  g.out << "What are you trying to do?\n";
  return false;
}

}  // namespace

namespace GB::commands {

bool route(const command_t& argv, GameObj& g) {
  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to 'cs' to a planet to examine routes.\n";
    return false;
  }

  if (argv.size() == 1) {
    show_all_routes(g);
    return true;
  }

  auto route_num = parse_route_index(argv[1], g);
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
  return configure_route_attribute(g, *route_num, argv[2], argv[3]);
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
