// SPDX-License-Identifier: Apache-2.0

/// \file fix.cc
/// \brief Deity fix-it utilities for planets and ships.

module;

import gb.entities;
import gb.presentation.empire_views;
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

  std::optional<GB::presentation::FixResult> result;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    auto make_res = [&](std::string_view prop, double val) {
      result = GB::presentation::FixResult{
          .target = GB::presentation::FixTarget::Planet,
          .property = std::string(prop),
          .display_property = std::string(prop),
          .value = val,
      };
    };
    if (argv[2] == "xpos") {
      if (opt_val) p.system_coordinates().x = static_cast<double>(*opt_val);
      make_res("xpos", p.system_coordinates().x);
      return;
    }
    if (argv[2] == "ypos") {
      if (opt_val) p.system_coordinates().y = static_cast<double>(*opt_val);
      make_res("ypos", p.system_coordinates().y);
      return;
    }
    if (argv[2] == "rtemp") {
      if (opt_val) p.rtemp() = *opt_val;
      make_res("rtemp", p.rtemp());
      return;
    }
    if (argv[2] == "temperature") {
      if (opt_val) p.temp() = *opt_val;
      make_res("temperature", p.temp());
      return;
    }
    if (argv[2] == "toxic") {
      if (opt_val) p.toxic() = *opt_val;
      make_res("toxic", p.toxic());
      return;
    }
    if (const auto cond = parse_condition(argv[2])) {
      if (opt_val) p.conditions(*cond) = *opt_val;
      make_res(to_string(*cond), p.conditions(*cond));
      return;
    }

    g.out << "No such option for 'fix planet'.\n";
  });
  if (!result) {
    return false;
  }
  g.present(*result);
  return true;
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
  if (argv[2] != "dead" && !parse_optional_int(argv, g, opt_val)) {
    return false;
  }

  std::optional<GB::presentation::FixResult> result;
  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& s) {
    const auto& race = *g.entity_manager.peek_race(s.owner());
    auto make_res = [&](std::string_view prop, std::string_view display_prop,
                        double val) {
      result = GB::presentation::FixResult{
          .target = GB::presentation::FixTarget::Ship,
          .property = std::string(prop),
          .display_property = std::string(display_prop),
          .value = val,
      };
    };
    if (argv[2] == "fuel") {
      if (opt_val) s.admin_override_fuel(*opt_val, race.mass);
      make_res("fuel", "fuel", s.fuel());
    } else if (argv[2] == "max_fuel") {
      if (opt_val) s.admin_override_max_fuel(*opt_val);
      make_res("max_fuel", "fuel", s.max_fuel());
    } else if (argv[2] == "destruct") {
      if (opt_val) s.admin_override_destruct(*opt_val, race.mass);
      make_res("destruct", "destruct", s.destruct());
    } else if (argv[2] == "resource") {
      if (opt_val) s.admin_override_resource(*opt_val, race.mass);
      make_res("resource", "resource", s.resource());
    } else if (argv[2] == "damage") {
      if (opt_val) s.admin_override_damage(*opt_val);
      make_res("damage", "damage", s.damage());
    } else if (argv[2] == "dead") {
      const std::string label = std::format("{}", s);
      s.admin_destroy();
      g.entity_manager.kill_ship(g.player(), s);
      result = GB::presentation::FixResult{
          .target = GB::presentation::FixTarget::Ship,
          .property = "dead",
          .display_property = "dead",
          .ship_destroyed = true,
          .ship_label = label,
      };
    } else {
      g.out << "No such option for 'fix ship'.\n";
    }
  });
  if (!result) {
    return false;
  }
  g.present(*result);
  return true;
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
