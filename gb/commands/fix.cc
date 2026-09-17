// SPDX-License-Identifier: Apache-2.0

/// \file fix.cc
/// \brief Deity fix-it utilities for planets and ships.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Parse an optional integer value argument (`argv[3]`) if provided.
 */
bool parse_optional_int(const command_t& argv, GameObj& g,
                        std::optional<int>& out_val) {
  if (argv.size() <= 3) {
    out_val = std::nullopt;
    return true;
  }
  auto parsed = scn::scan<int>(argv[3], "{}");
  if (!parsed) {
    g.out << "Invalid numeric value.\n";
    return false;
  }
  out_val = parsed->value();
  return true;
}

/**
 * @brief Apply or inspect deity overrides on the current planet.
 */
bool fix_planet(const command_t& argv, GameObj& g) {
  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "Change scope to the planet first.\n";
    return false;
  }

  std::optional<int> opt_val;
  if (!parse_optional_int(argv, g, opt_val)) {
    return false;
  }

  bool ok = false;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    if (argv[2] == "xpos") {
      if (opt_val) p.xpos() = static_cast<double>(*opt_val);
      g.out << std::format("xpos = {}\n", p.xpos());
      ok = true;
      return;
    }
    if (argv[2] == "ypos") {
      if (opt_val) p.ypos() = static_cast<double>(*opt_val);
      g.out << std::format("ypos = {}\n", p.ypos());
      ok = true;
      return;
    }

    if (const auto cond = parse_condition(argv[2])) {
      if (opt_val) p.conditions(*cond) = *opt_val;
      g.out << std::format("{} = {}\n", *cond, p.conditions(*cond));
      ok = true;
      return;
    }

    g.out << "No such option for 'fix planet'.\n";
  });
  return ok;
}

/**
 * @brief Apply or inspect deity overrides on the current ship.
 */
bool fix_ship(const command_t& argv, GameObj& g) {
  if (g.level() != ScopeLevel::LEVEL_SHIP) {
    g.out << "Change scope to the ship you wish to fix.\n";
    return false;
  }

  std::optional<int> opt_val;
  if (argv[2] != "alive" && argv[2] != "dead" &&
      !parse_optional_int(argv, g, opt_val)) {
    return false;
  }

  bool ok = false;
  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& s) {
    const auto& race = *g.entity_manager.peek_race(s.owner());
    if (argv[2] == "fuel") {
      if (opt_val) s.admin_override_fuel(*opt_val, race.mass);
      g.out << std::format("fuel = {}\n", s.fuel());
    } else if (argv[2] == "max_fuel") {
      if (opt_val) s.admin_override_max_fuel(*opt_val);
      g.out << std::format("fuel = {}\n", s.max_fuel());
    } else if (argv[2] == "destruct") {
      if (opt_val) s.admin_override_destruct(*opt_val, race.mass);
      g.out << std::format("destruct = {}\n", s.destruct());
    } else if (argv[2] == "resource") {
      if (opt_val) s.admin_override_resource(*opt_val, race.mass);
      g.out << std::format("resource = {}\n", s.resource());
    } else if (argv[2] == "damage") {
      if (opt_val) s.admin_override_damage(*opt_val);
      g.out << std::format("damage = {}\n", s.damage());
    } else if (argv[2] == "alive") {
      s.admin_resurrect();
      g.out << std::format("{} resurrected\n", s);
    } else if (argv[2] == "dead") {
      s.admin_destroy();
      g.out << std::format("{} destroyed\n", s);
    } else {
      g.out << "No such option for 'fix ship'.\n";
      return;
    }
    ok = true;
  });
  return ok;
}

}  // namespace

namespace GB::commands {

/** Deity fix-it utilities */
bool fix(const command_t& argv, GameObj& g) {
  if (argv[1] == "planet") {
    return fix_planet(argv, g);
  }
  if (argv[1] == "ship") {
    return fix_ship(argv, g);
  }
  g.out << "Fix what?\n";
  return false;
}

const CommandDescriptor fix_cmd{
    .name = "fix",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "fix <planet|ship> <property> [<value>]",
    .description = "Deity fix-it utilities for planets and ships",
    .handler = &fix,
};

}  // namespace GB::commands
