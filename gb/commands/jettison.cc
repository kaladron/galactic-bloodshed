// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

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
void jettison(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 0;
  int amt;
  char commod;

  if (argv.size() < 2) {
    g.out << "Jettison what?\n";
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    const Ship& s = ship_handle.peek();

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

    if (s.owner() != Playernum || !s.alive()) {
      continue;
    }
    if (landed(s)) {
      g.out << "Ship is landed, cannot jettison.\n";
      continue;
    }
    if (!s.active()) {
      g.out << std::format("{} is irradiated and inactive.\n",
                           ship_to_string(s));
      continue;
    }
    if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      const auto* universe = g.entity_manager.peek_universe();
      if (!enufAP(g.entity_manager, Playernum, Governor,
                  universe->AP[Playernum.value - 1], APcount)) {
        continue;
      }
    } else {
      const auto* star = g.entity_manager.peek_star(s.storbits());
      if (!enufAP(g.entity_manager, Playernum, Governor, star->AP(Playernum),
                  APcount)) {
        continue;
      }
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
        }
        break;
      case 'c':
        if ((amt = jettison_check(g, amt, (int)(ship.popn()))) > 0) {
          ship.popn() -= amt;
          ship.mass() -= amt * g.race->mass;
          g.out << std::format("{} crew {} into deep space.\n", amt,
                               (amt == 1) ? "hurls itself" : "hurl themselves");
          g.out << std::format("Complement of {} is now {}.\n",
                               ship_to_string(ship), ship.popn());
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
        }
        break;
      case 'd':
        if ((amt = jettison_check(g, amt, (int)(ship.destruct()))) > 0) {
          use_destruct(ship, amt);
          g.out << std::format("{} destruct jettisoned.\n", amt);
          if (!max_crew(ship)) {
            g.out << std::format("\n{} ", ship_to_string(ship));
            if (ship.destruct()) {
              g.out << "still boobytrapped.\n";
            } else {
              g.out << "no longer boobytrapped.\n";
            }
          }
        }
        break;
      case 'f':
        if ((amt = jettison_check(g, amt, (int)(ship.fuel()))) > 0) {
          use_fuel(ship, (double)amt);
          g.out << std::format("{} fuel jettisoned.\n", amt);
        }
        break;
      case 'r':
        if ((amt = jettison_check(g, amt, (int)(ship.resource()))) > 0) {
          use_resource(ship, amt);
          g.out << std::format("{} resources jettisoned.\n", amt);
        }
        break;
      default:
        g.out << "No such commodity valid.\n";
        return;
    }
  }
}
}  // namespace GB::commands
