// SPDX-License-Identifier: Apache-2.0

/// \file fix.cc

module;

import gblib;
import std;
#undef stdout

module commands;

namespace GB::commands {
/** Deity fix-it utilities */
bool fix(const command_t& argv, GameObj& g) {
  if (argv[1] == "planet") {
    if (g.level() != ScopeLevel::LEVEL_PLAN) {
      g.out << "Change scope to the planet first.\n";
      return false;
    }
    bool ok = false;
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
      if (argv[2] == "xpos") {
        if (argv.size() > 3) p.xpos() = (double)std::stoi(argv[3]);
        g.out << std::format("xpos = {}\n", p.xpos());
      } else if (argv[2] == "ypos") {
        if (argv.size() > 3) p.ypos() = (double)std::stoi(argv[3]);
        g.out << std::format("ypos = {}\n", p.ypos());
      } else if (argv[2] == "ships") {
        if (argv.size() > 3) p.ships() = std::stoi(argv[3]);
        g.out << std::format("ships = {}\n", p.ships());
      } else if (argv[2] == "rtemp") {
        if (argv.size() > 3) p.conditions(RTEMP) = std::stoi(argv[3]);
        g.out << std::format("RTEMP = {}\n", p.conditions(RTEMP));
      } else if (argv[2] == "temperature") {
        if (argv.size() > 3) p.conditions(TEMP) = std::stoi(argv[3]);
        g.out << std::format("TEMP = {}\n", p.conditions(TEMP));
      } else if (argv[2] == "methane") {
        if (argv.size() > 3) p.conditions(METHANE) = std::stoi(argv[3]);
        g.out << std::format("METHANE = {}\n", p.conditions(METHANE));
      } else if (argv[2] == "oxygen") {
        if (argv.size() > 3) p.conditions(OXYGEN) = std::stoi(argv[3]);
        g.out << std::format("OXYGEN = {}\n", p.conditions(OXYGEN));
      } else if (argv[2] == "co2") {
        if (argv.size() > 3) p.conditions(CO2) = std::stoi(argv[3]);
        g.out << std::format("CO2 = {}\n", p.conditions(CO2));
      } else if (argv[2] == "hydrogen") {
        if (argv.size() > 3) p.conditions(HYDROGEN) = std::stoi(argv[3]);
        g.out << std::format("HYDROGEN = {}\n", p.conditions(HYDROGEN));
      } else if (argv[2] == "nitrogen") {
        if (argv.size() > 3) p.conditions(NITROGEN) = std::stoi(argv[3]);
        g.out << std::format("NITROGEN = {}\n", p.conditions(NITROGEN));
      } else if (argv[2] == "sulfur") {
        if (argv.size() > 3) p.conditions(SULFUR) = std::stoi(argv[3]);
        g.out << std::format("SULFUR = {}\n", p.conditions(SULFUR));
      } else if (argv[2] == "helium") {
        if (argv.size() > 3) p.conditions(HELIUM) = std::stoi(argv[3]);
        g.out << std::format("HELIUM = {}\n", p.conditions(HELIUM));
      } else if (argv[2] == "other") {
        if (argv.size() > 3) p.conditions(OTHER) = std::stoi(argv[3]);
        g.out << std::format("OTHER = {}\n", p.conditions(OTHER));
      } else if (argv[2] == "toxic") {
        if (argv.size() > 3) p.conditions(TOXIC) = std::stoi(argv[3]);
        g.out << std::format("TOXIC = {}\n", p.conditions(TOXIC));
      } else {
        g.out << "No such option for 'fix planet'.\n";
        return;
      }
      ok = true;
    });
    return ok;
  }
  if (argv[1] == "ship") {
    if (g.level() != ScopeLevel::LEVEL_SHIP) {
      g.out << "Change scope to the ship you wish to fix.\n";
      return false;
    }
    bool ok = false;
    g.entity_manager.mutate_ship(g.shipno(), [&](Ship& s) {
      if (argv[2] == "fuel") {
        if (argv.size() > 3) s.fuel() = (double)std::stoi(argv[3]);
        g.out << std::format("fuel = {}\n", s.fuel());
      } else if (argv[2] == "max_fuel") {
        if (argv.size() > 3) s.max_fuel() = std::stoi(argv[3]);
        g.out << std::format("fuel = {}\n", s.max_fuel());
      } else if (argv[2] == "destruct") {
        if (argv.size() > 3) s.destruct() = std::stoi(argv[3]);
        g.out << std::format("destruct = {}\n", s.destruct());
      } else if (argv[2] == "resource") {
        if (argv.size() > 3) s.resource() = std::stoi(argv[3]);
        g.out << std::format("resource = {}\n", s.resource());
      } else if (argv[2] == "damage") {
        if (argv.size() > 3) s.damage() = std::stoi(argv[3]);
        g.out << std::format("damage = {}\n", s.damage());
      } else if (argv[2] == "alive") {
        s.alive() = 1;
        s.damage() = 0;
        g.out << std::format("{} resurrected\n", s);
      } else if (argv[2] == "dead") {
        s.alive() = 0;
        s.damage() = 100;
        g.out << std::format("{} destroyed\n", s);
      } else {
        g.out << "No such option for 'fix ship'.\n";
        return;
      }
      ok = true;
    });
    return ok;
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
