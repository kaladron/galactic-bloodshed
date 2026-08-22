// SPDX-License-Identifier: Apache-2.0

/// \file give.cc
/// \brief Transfer ship ownership to a mutual ally.

module;

import session;
import gblib;
import notification;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool give(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  player_t who = get_player(g.entity_manager, argv[1]);
  if (who.value == 0) {
    g.out << "No such player.\n";
    return false;
  }

  auto alien_handle = g.entity_manager.get_race(who);
  if (!alien_handle.get()) {
    g.out << "Race not found.\n";
    return false;
  }
  auto& alien = *alien_handle;
  const auto& race = *g.race;
  if (alien.Guest && !race.God) {
    g.out << "You can't give this player anything.\n";
    return false;
  }
  /* check to see if both players are mutually allied */
  if (!race.God &&
      !(isset(race.allied, who) && isset(alien.allied, Playernum))) {
    g.out << "You two are not mutually allied.\n";
    return false;
  }
  auto shipno = string_to_shipnum(argv[2]);
  if (!shipno) {
    g.out << "Illegal ship number.\n";
    return false;
  }

  try {
    g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "No such ship.\n";
    return false;
  }
  auto ship_handle = g.entity_manager.get_ship(*shipno);
  auto& ship = *ship_handle;

  if (ship.owner() != Playernum || !ship.alive()) {
    DontOwnErr(g.entity_manager, Playernum, g.governor(), *shipno);
    return false;
  }
  if (ship.type() == ShipType::STYPE_POD) {
    g.out << "You cannot change the ownership of spore pods.\n";
    return false;
  }

  if ((ship.popn() + ship.troops()) && !race.God) {
    g.out << "You can't give this ship away while it has crew/mil on board.\n";
    return false;
  }
  if (ship.ships() != 0 && !race.God) {
    g.out
        << "You can't give away this ship, it has other ships loaded on it.\n";
    return false;
  }

  if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (!g.deduct_univ_ap(5)) {
      g.out << "You don't have enough universe action points.\n";
      return false;
    }
  } else {
    if (!g.deduct_ap(ship.storbits(), 5)) {
      g.out << "You don't have enough action points in that system.\n";
      return false;
    }
  }

  ship.owner() = who;
  ship.governor() = 0; /* give to the leader */
  capture_stuff(ship, g);

  /* set inhabited/explored bits */
  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      break;
    case ScopeLevel::LEVEL_STAR: {
      auto star_handle = g.entity_manager.get_star(ship.storbits());
      auto& star = *star_handle;
      setbit(star.explored(), who);
      break;
    }
    case ScopeLevel::LEVEL_PLAN: {
      auto star_handle = g.entity_manager.get_star(ship.storbits());
      auto& star = *star_handle;
      setbit(star.explored(), who);

      auto planet_handle =
          g.entity_manager.get_planet(ship.storbits(), ship.pnumorbits());
      auto& planet = *planet_handle;
      planet.info(who).explored = 1;
      break;
    }
    default:
      g.out << "Something wrong with this ship's scope.\n";
      return false;
  }

  g.out << "Owner changed.\n";
  std::string givemsg =
      std::format("{} [{}] gave you {} at {}.\n", race.name, Playernum, ship,
                  prin_ship_orbits(g.entity_manager, ship));
  warn_player(g.session_registry, g.entity_manager, who, 0, givemsg);

  if (!race.God) {
    std::string postmsg = std::format("{} [{}] gives {} [{}] a ship.\n",
                                      race.name, Playernum, alien.name, who);
    post(g.entity_manager, postmsg, NewsType::TRANSFER);
  }
  return true;
}

const CommandDescriptor give_cmd{
    .name = "give",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "give <race> <#ship>",
    .description = "Transfer ownership of an uncrewed ship to a mutual ally",
    .handler = &give,
};

}  // namespace GB::commands
