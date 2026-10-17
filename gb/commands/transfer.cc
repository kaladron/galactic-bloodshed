// SPDX-License-Identifier: Apache-2.0

/// \file transfer.cc
/// \brief Transfer command implementation.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {
bool transfer(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  char commod = 0;

  std::string star_name;
  std::string planet_name;
  g.entity_manager.with_star(g.snum(), [&](const Star& star) {
    star_name = star.get_name();
    planet_name = star.get_planet_name(g.pnum());
  });

  auto player = get_player(g.entity_manager, argv[1]);
  if (player == 0) {
    g.out << "No such player.\n";
    return false;
  }

  auto scan_result = scn::scan<char>(argv[2], "{}");
  if (!scan_result) {
    g.out << "Invalid commodity type.\n";
    return false;
  }
  commod = scan_result->value();

  auto parsed_give = scn::scan<resource_t>(argv[3], "{}");
  if (!parsed_give || parsed_give->value() <= 0) {
    g.out << "You must give a positive amount.\n";
    return false;
  }
  resource_t give = parsed_give->value();

  bool ok = false;
  CommodType type = CommodType::RESOURCE;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
    switch (commod) {
      case 'r': {
        if (give > planet.info(Playernum).resource) {
          g.out << std::format("You don't have {} on this planet.\n", give);
          return;
        }
        planet.info(Playernum).resource -= give;
        planet.info(player).resource += give;
        type = CommodType::RESOURCE;
      } break;
      case 'x':
      case '&': {
        if (give > planet.info(Playernum).crystals) {
          g.out << std::format("You don't have {} on this planet.\n", give);
          return;
        }
        planet.info(Playernum).crystals -= give;
        planet.info(player).crystals += give;
        type = CommodType::CRYSTAL;
      } break;
      case 'f': {
        if (give > planet.info(Playernum).fuel) {
          g.out << std::format("You don't have {} fuel on this planet.\n",
                               give);
          return;
        }
        planet.info(Playernum).fuel -= give;
        planet.info(player).fuel += give;
        type = CommodType::FUEL;
      } break;
      case 'd': {
        if (give > planet.info(Playernum).destruct) {
          g.out << std::format("You don't have {} destruct on this planet.\n",
                               give);
          return;
        }
        planet.info(Playernum).destruct -= give;
        planet.info(player).destruct += give;
        type = CommodType::DESTRUCT;
      } break;
      default:
        g.out << "What?\n";
        return;
    }
    ok = true;
  });

  if (ok) {
    const presentation::TransferResult result{
        .star_name = star_name,
        .planet_name = planet_name,
        .type = type,
        .amount = give,
        .sender = Playernum,
        .recipient = player,
    };
    g.present(result);
    warn_race(g.session_registry, g.entity_manager, player,
              presentation::render_transfer_result(result));
  }

  return ok;
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