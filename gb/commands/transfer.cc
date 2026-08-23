// SPDX-License-Identifier: Apache-2.0

/// \file transfer.cc
/// \brief Transfer command implementation.

module;

import gblib;
import scnlib;
import std;
import notification;
import session;
#undef stdout

module commands;

namespace GB::commands {
bool transfer(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  char commod = 0;

  const auto* star = g.entity_manager.peek_star(g.snum());

  auto player = get_player(g.entity_manager, argv[1]);
  if (player == 0) {
    g.out << "No such player.\n";
    return false;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  auto& planet = *planet_handle;

  auto scan_result = scn::scan<char>(argv[2], "{}");
  if (!scan_result) {
    g.out << "Invalid commodity type.\n";
    return false;
  }
  commod = scan_result->value();
  // TODO(jeffbailey): May throw an exception on a negative number.
  resource_t give = std::stoul(argv[3]);

  std::string starplanet =
      std::format("{}/{}:", star->get_name(), star->get_planet_name(g.pnum()));
  switch (commod) {
    case 'r': {
      if (give > planet.info(Playernum).resource) {
        g.out << std::format("You don't have {} on this planet.\n", give);
        return false;
      }
      planet.info(Playernum).resource -= give;
      planet.info(player).resource += give;
      std::string message = std::format(
          "{} {} resources transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.session_registry, g.entity_manager, player, message);
    } break;
    case 'x':
    case '&': {
      if (give > planet.info(Playernum).crystals) {
        g.out << std::format("You don't have {} on this planet.\n", give);
        return false;
      }
      planet.info(Playernum).crystals -= give;
      planet.info(player).crystals += give;
      std::string message = std::format(
          "{} {} crystal(s) transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.session_registry, g.entity_manager, player, message);
    } break;
    case 'f': {
      if (give > planet.info(Playernum).fuel) {
        g.out << std::format("You don't have {} fuel on this planet.\n", give);
        return false;
      }
      planet.info(Playernum).fuel -= give;
      planet.info(player).fuel += give;
      std::string message =
          std::format("{} {} fuel transferred from player {} to player #{}\n",
                      starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.session_registry, g.entity_manager, player, message);
    } break;
    case 'd': {
      if (give > planet.info(Playernum).destruct) {
        g.out << std::format("You don't have {} destruct on this planet.\n",
                             give);
        return false;
      }
      planet.info(Playernum).destruct -= give;
      planet.info(player).destruct += give;
      std::string message = std::format(
          "{} {} destruct transferred from player {} to player #{}\n",
          starplanet, give, Playernum, player);
      g.out << message;
      warn_race(g.session_registry, g.entity_manager, player, message);
    } break;
    default:
      g.out << "What?\n";
      return false;
  }

  return true;
}

const CommandDescriptor transfer_cmd{
    .name = "transfer",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 4,
    .syntax = "transfer <player> <commodity> <amount>",
    .description = "Transfer supplies to alien stockpiles on a planet",
    .handler = &transfer,
};

}  // namespace GB::commands