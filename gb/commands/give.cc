// SPDX-License-Identifier: Apache-2.0

module;

import session;
import gblib;
import notification;
import std;

module commands;

namespace GB::commands {
void give(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 5;
  player_t who;

  if (!(who = get_player(g.entity_manager, argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (Governor != 0) {
    g.out << "You are not authorized to do that.\n";
    return;
  }
  auto alien_handle = g.entity_manager.get_race(who);
  if (!alien_handle.get()) {
    g.out << "Race not found.\n";
    return;
  }
  auto& alien = *alien_handle;
  const auto& race = *g.race;
  if (alien.Guest && !race.God) {
    g.out << "You can't give this player anything.\n";
    return;
  }
  if (race.Guest) {
    g.out << "You can't give anyone anything.\n";
    return;
  }
  /* check to see if both players are mutually allied */
  if (!race.God &&
      !(isset(race.allied, who) && isset(alien.allied, Playernum))) {
    g.out << "You two are not mutually allied.\n";
    return;
  }
  auto shipno = string_to_shipnum(argv[2]);
  if (!shipno) {
    g.out << "Illegal ship number.\n";
    return;
  }

  auto ship_handle = g.entity_manager.get_ship(*shipno);
  if (!ship_handle.get()) {
    g.out << "No such ship.\n";
    return;
  }
  auto& ship = *ship_handle;

  if (ship.owner() != Playernum || !ship.alive()) {
    DontOwnErr(g.entity_manager, Playernum, Governor, *shipno);
    return;
  }
  if (ship.type() == ShipType::STYPE_POD) {
    g.out << "You cannot change the ownership of spore pods.\n";
    return;
  }

  if ((ship.popn() + ship.troops()) && !race.God) {
    g.out << "You can't give this ship away while it has crew/mil on board.\n";
    return;
  }
  if (ship.ships() && !race.God) {
    g.out
        << "You can't give away this ship, it has other ships loaded on it.\n";
    return;
  }
  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_UNIV: {
      const auto* univ = g.entity_manager.peek_universe();
      if (!enufAP(g.entity_manager, Playernum, Governor,
                  univ->AP[Playernum - 1], APcount)) {
        return;
      }
      break;
    }
    default: {
      const auto& star = *g.entity_manager.peek_star(g.snum());
      if (!enufAP(g.entity_manager, Playernum, Governor, star.AP(Playernum - 1),
                  APcount)) {
        return;
      }
      break;
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
      planet.info(who - 1).explored = 1;
      break;
    }
    default:
      g.out << "Something wrong with this ship's scope.\n";
      return;
  }

  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
      return;
    default:
      deductAPs(g, APcount, g.snum());
      break;
  }
  g.out << "Owner changed.\n";
  std::string givemsg = std::format("{} [{}] gave you {} at {}.\n", race.name,
                                    Playernum, ship_to_string(ship),
                                    prin_ship_orbits(g.entity_manager, ship));
  warn_player(g.session_registry, g.entity_manager, who, 0, givemsg);

  if (!race.God) {
    std::string postmsg = std::format("{} [{}] gives {} [{}] a ship.\n",
                                      race.name, Playernum, alien.name, who);
    post(g.entity_manager, postmsg, NewsType::TRANSFER);
  }
}
}  // namespace GB::commands
