// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void transfer(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  char commod = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You need to be in planet scope to do this.\n";
    return;
  }

  if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1), APcount))
    return;

  auto player = get_player(argv[1]);
  if (player == 0) {
    g.out << "No such player.\n";
    return;
  }

  auto planet = getplanet(g.snum, g.pnum);

  sscanf(argv[2].c_str(), "%c", &commod);
  // TODO(jeffbailey): May throw an exception on a negative number.
  resource_t give = std::stoul(argv[3]);

  std::string starplanet = std::format("{}/{}:", stars[g.snum].get_name(),
                                       stars[g.snum].get_planet_name(g.pnum));
  switch (commod) {
    case 'r': {
      if (give > planet.info[Playernum - 1].resource) {
        g.out << std::format("You don't have {} on this planet.\n", give);
        return;
      }
      planet.info[Playernum - 1].resource -= give;
      planet.info[player - 1].resource += give;
      std::string message = std::format(
          "{} {} resources transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(player, message);
    } break;
    case 'x':
    case '&': {
      if (give > planet.info[Playernum - 1].crystals) {
        g.out << std::format("You don't have {} on this planet.\n", give);
        return;
      }
      planet.info[Playernum - 1].crystals -= give;
      planet.info[player - 1].crystals += give;
      std::string message = std::format(
          "{} {} crystal(s) transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(player, message);
    } break;
    case 'f': {
      if (give > planet.info[Playernum - 1].fuel) {
        g.out << std::format("You don't have {} fuel on this planet.\n", give);
        return;
      }
      planet.info[Playernum - 1].fuel -= give;
      planet.info[player - 1].fuel += give;
      std::string message =
          std::format("{} {} fuel transferred from player {} to player #{}\n",
                      starplanet, give, Playernum, player);
      g.out << message;
      warn_race(player, message);
    } break;
    case 'd': {
      if (give > planet.info[Playernum - 1].destruct) {
        g.out << std::format("You don't have {} destruct on this planet.\n",
                             give);
        return;
      }
      planet.info[Playernum - 1].destruct -= give;
      planet.info[player - 1].destruct += give;
      std::string message = std::format(
          "{} {} destruct transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(player, message);
    } break;
    default:
      g.out << "What?\n";
      return;
  }

  putplanet(planet, stars[g.snum], g.pnum);

  deductAPs(g, APcount, g.snum);
}
}  // namespace GB::commands