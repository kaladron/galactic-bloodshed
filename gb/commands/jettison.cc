// SPDX-License-Identifier: Apache-2.0

/// \file jettison.cc
/// \brief Functions for jettisoning cargo into deep space.

module commands;

import gb.entities;
import gb.services;
import std;

namespace {
int jettison_check(GameObj& g, int amt, int max) {
  if (amt == 0) amt = max;
  if (amt < 0) {
    g.out << "Nice try.\n";
    return -1;
  }
  if (amt > max) {
    g.out << std::format("You can jettison at most {}\n", max);
    return -1;
  }
  return amt;
}
}  // namespace

namespace GB::commands {
bool jettison(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  int amt;
  char commod;
  bool success = false;

  if (argv.size() < 3) {
    g.out << "Jettison what?\n";
    return false;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    const Ship& s = ship_handle.peek();

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

    if (s.owner() != Playernum || !s.alive()) {
      continue;
    }
    if (s.is_landed()) {
      g.out << "Ship is landed, cannot jettison.\n";
      continue;
    }
    if (!s.active()) {
      g.out << std::format("{} is irradiated and inactive.\n", s);
      continue;
    }

    if (argv.size() > 3)
      amt = std::stoi(argv[3]);
    else
      amt = 0;

    // Now get mutable access for modifications
    Ship& ship = *ship_handle;

    commod = argv[2][0];
    switch (commod) {
      case 'x':
        if ((amt = jettison_check(g, amt, (int)(ship.crystals()))) > 0) {
          ship.crystals() -= amt;
          g.out << std::format("{} crystal{} jettisoned.\n", amt,
                               (amt == 1) ? "" : "s");
          success = true;
        }
        break;
      case 'c':
        if ((amt = jettison_check(g, amt, (int)(ship.popn()))) > 0) {
          ship.popn() -= amt;
          ship.mass() -= amt * g.race->mass;
          g.out << std::format("{} crew {} into deep space.\n", amt,
                               (amt == 1) ? "hurls itself" : "hurl themselves");
          g.out << std::format("Complement of {} is now {}.\n", ship,
                               ship.popn());
          success = true;
        }
        break;
      case 'm':
        if ((amt = jettison_check(g, amt, (int)(ship.troops()))) > 0) {
          g.out << std::format("{} military {} into deep space.\n", amt,
                               (amt == 1) ? "hurls itself" : "hurl themselves");
          g.out << std::format("Complement of ship #{} is now {}.\n",
                               ship.number(), ship.troops() - amt);
          ship.troops() -= amt;
          ship.mass() -= amt * g.race->mass;
          success = true;
        }
        break;
      case 'd':
        if ((amt = jettison_check(g, amt, (int)(ship.destruct()))) > 0) {
          use_destruct(ship, amt);
          g.out << std::format("{} destruct jettisoned.\n", amt);
          if (!ship.max_crew_capacity()) {
            g.out << std::format("\n{} ", ship);
            if (ship.destruct()) {
              g.out << "still boobytrapped.\n";
            } else {
              g.out << "no longer boobytrapped.\n";
            }
          }
          success = true;
        }
        break;
      case 'f':
        if ((amt = jettison_check(g, amt, (int)(ship.fuel()))) > 0) {
          use_fuel(ship, (double)amt);
          g.out << std::format("{} fuel jettisoned.\n", amt);
          success = true;
        }
        break;
      case 'r':
        if ((amt = jettison_check(g, amt, (int)(ship.resource()))) > 0) {
          use_resource(ship, amt);
          g.out << std::format("{} resources jettisoned.\n", amt);
          success = true;
        }
        break;
      default:
        g.out << "No such commodity valid.\n";
        return false;
    }
  }
  return success;
}

const CommandDescriptor jettison_cmd{
    .name = "jettison",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "jettison <ship> <commodity> [<amount>]",
    .description = "Unload commodities from a ship into space",
    .handler = &jettison,
};

}  // namespace GB::commands
