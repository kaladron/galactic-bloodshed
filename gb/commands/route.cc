// SPDX-License-Identifier: Apache-2.0

/// \file route.cc
/// \brief Set and view automated shipping routes for planets.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool route(const command_t& argv, GameObj& g) {
  // TODO(jeffbailey): This seems to segfault with no args.
  player_t Playernum = g.player();
  // TODO(jeffbailey): ap_t APcount = 0;
  int i;
  starnum_t star;
  planetnum_t planet;
  CommodityManifest load;
  CommodityManifest unload;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to 'cs' to a planet to examine routes.\n";
    return false;
  }

  if (argv.size() == 1) { /* display all shipping routes that are active */
    g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
      for (i = 1; i <= MAX_ROUTES; i++)
        if (p.info(Playernum).route[i - 1].set) {
          star = p.info(Playernum).route[i - 1].dest_star;
          planet = p.info(Playernum).route[i - 1].dest_planet;
          load = p.info(Playernum).route[i - 1].load;
          unload = p.info(Playernum).route[i - 1].unload;
          std::string load_flags;
          load_flags += load.fuel ? 'f' : ' ';
          load_flags += load.destruct ? 'd' : ' ';
          load_flags += load.resources ? 'r' : ' ';
          load_flags += load.crystals ? 'x' : ' ';

          std::string unload_flags;
          unload_flags += unload.fuel ? 'f' : ' ';
          unload_flags += unload.destruct ? 'd' : ' ';
          unload_flags += unload.resources ? 'r' : ' ';
          unload_flags += unload.crystals ? 'x' : ' ';

          const auto* dest_star = g.entity_manager.peek_star(star);
          g.out << std::format(
              "{:2}  land {:2},{:2}   load: {}  unload: {}  -> {}/{}\n", i,
              p.info(Playernum).route[i - 1].dest_coords.x,
              p.info(Playernum).route[i - 1].dest_coords.y, load_flags,
              unload_flags, dest_star ? dest_star->get_name() : "???",
              (dest_star && planet < dest_star->numplanets())
                  ? dest_star->get_planet_name(planet)
                  : "???");
        }
    });
    g.out << "Done.\n";
    return true;
  }
  if (argv.size() == 2) {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return false;
    }
    g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
      if (p.info(Playernum).route[i - 1].set) {
        star = p.info(Playernum).route[i - 1].dest_star;
        planet = p.info(Playernum).route[i - 1].dest_planet;
        load = p.info(Playernum).route[i - 1].load;
        unload = p.info(Playernum).route[i - 1].unload;
        std::string load_flags;
        if (load.any()) {
          if (load.fuel) load_flags += 'f';
          if (load.destruct) load_flags += 'd';
          if (load.resources) load_flags += 'r';
          if (load.crystals) load_flags += 'x';
        }
        std::string unload_flags;
        if (unload.any()) {
          if (unload.fuel) unload_flags += 'f';
          if (unload.destruct) unload_flags += 'd';
          if (unload.resources) unload_flags += 'r';
          if (unload.crystals) unload_flags += 'x';
        }
        const auto* dest_star = g.entity_manager.peek_star(star);
        g.out << std::format(
            "{:2}  land {:2},{:2}   {}{}  -> {}/{}\n", i,
            p.info(Playernum).route[i - 1].dest_coords.x,
            p.info(Playernum).route[i - 1].dest_coords.y,
            (load.any() ? std::format("load: {}", load_flags) : std::string{}),
            (unload.any() ? std::format("  unload: {}", unload_flags)
                          : std::string{}),
            dest_star ? dest_star->get_name() : "???",
            (dest_star && planet < dest_star->numplanets())
                ? dest_star->get_planet_name(planet)
                : "???");
      }
    });
    g.out << "Done.\n";
    return true;
  }
  if (argv.size() == 3) {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return false;
    }
    if (argv[2] == "activate") {
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
        p.info(Playernum).route[i - 1].set = true;
      });
      g.out << "Set.\n";
    } else if (argv[2] == "deactivate") {
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
        p.info(Playernum).route[i - 1].set = false;
      });
      g.out << "Set.\n";
    } else {
      Place where{g, argv[2], true};
      if (!where.err) {
        if (where.level != ScopeLevel::LEVEL_PLAN) {
          g.out << "You have to designate a planet.\n";
          return false;
        }
        g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
          p.info(Playernum).route[i - 1].dest_star = where.snum;
          p.info(Playernum).route[i - 1].dest_planet = where.pnum;
        });
        g.out << "Set.\n";
      } else {
        g.out << "Illegal destination.\n";
        return false;
      }
    }
    return true;
  } else {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return false;
    }
    if (argv[2] == "land") {
      auto coords_opt = Coordinates::parse(argv[3]);
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
        p.info(Playernum).route[i - 1].dest_coords = coords;
      });
      if (!valid) {
        g.out << "Bad sector coordinates.\n";
        return false;
      }
    } else if (argv[2] == "load") {
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
        p.info(Playernum).route[i - 1].load = {};
        for (char c : argv[3]) {
          if (c == 'f') p.info(Playernum).route[i - 1].load.fuel = true;
          if (c == 'd') p.info(Playernum).route[i - 1].load.destruct = true;
          if (c == 'r') p.info(Playernum).route[i - 1].load.resources = true;
          if (c == 'x') p.info(Playernum).route[i - 1].load.crystals = true;
        }
      });
    } else if (argv[2] == "unload") {
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
        p.info(Playernum).route[i - 1].unload = {};
        for (char c : argv[3]) {
          if (c == 'f') p.info(Playernum).route[i - 1].unload.fuel = true;
          if (c == 'd') p.info(Playernum).route[i - 1].unload.destruct = true;
          if (c == 'r') p.info(Playernum).route[i - 1].unload.resources = true;
          if (c == 'x') p.info(Playernum).route[i - 1].unload.crystals = true;
        }
      });
    } else {
      g.out << "What are you trying to do?\n";
      return false;
    }
    g.out << "Set.\n";
  }
  return true;
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
