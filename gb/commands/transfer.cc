// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import scnlib;
import std;

module commands;

namespace GB::commands {
void transfer(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 1;
  char commod = 0;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You need to be in planet scope to do this.\n";
    return;
  }

  const auto* star = g.entity_manager.peek_star(g.snum());
  if (!enufAP(Playernum, Governor, star->AP(Playernum - 1), APcount)) return;

  auto player = get_player(g.entity_manager, argv[1]);
  if (player == 0) {
    g.out << "No such player.\n";
    return;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  auto& planet = *planet_handle;

  auto scan_result = scn::scan<char>(argv[2], "{}");
  if (!scan_result) {
    g.out << "Invalid commodity type.\n";
    return;
  }
  commod = scan_result->value();
  // TODO(jeffbailey): May throw an exception on a negative number.
  resource_t give = std::stoul(argv[3]);

  std::string starplanet =
      std::format("{}/{}:", star->get_name(), star->get_planet_name(g.pnum()));
  switch (commod) {
    case 'r': {
      if (give > planet.info(Playernum - 1).resource) {
        g.out << std::format("You don't have {} on this planet.\n", give);
        return;
      }
      planet.info(Playernum - 1).resource -= give;
      planet.info(player - 1).resource += give;
      std::string message = std::format(
          "{} {} resources transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.entity_manager, player, message);
    } break;
    case 'x':
    case '&': {
      if (give > planet.info(Playernum - 1).crystals) {
        g.out << std::format("You don't have {} on this planet.\n", give);
        return;
      }
      planet.info(Playernum - 1).crystals -= give;
      planet.info(player - 1).crystals += give;
      std::string message = std::format(
          "{} {} crystal(s) transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.entity_manager, player, message);
    } break;
    case 'f': {
      if (give > planet.info(Playernum - 1).fuel) {
        g.out << std::format("You don't have {} fuel on this planet.\n", give);
        return;
      }
      planet.info(Playernum - 1).fuel -= give;
      planet.info(player - 1).fuel += give;
      std::string message =
          std::format("{} {} fuel transferred from player {} to player #{}\n",
                      starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.entity_manager, player, message);
    } break;
    case 'd': {
      if (give > planet.info(Playernum - 1).destruct) {
        g.out << std::format("You don't have {} destruct on this planet.\n",
                             give);
        return;
      }
      planet.info(Playernum - 1).destruct -= give;
      planet.info(player - 1).destruct += give;
      std::string message = std::format(
          "{} {} destruct transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.entity_manager, player, message);
    } break;
    default:
      g.out << "What?\n";
      return;
  }

  deductAPs(g, APcount, g.snum());
}
}  // namespace GB::commands