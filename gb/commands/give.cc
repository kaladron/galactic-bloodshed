// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void give(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 5;
  player_t who;

  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (Governor) {
    g.out << "You are not authorized to do that.\n";
    return;
  }
  auto &alien = races[who - 1];
  auto &race = races[Playernum - 1];
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

  auto ship = getship(*shipno);
  if (!ship) {
    g.out << "No such ship.\n";
    return;
  }

  if (ship->owner != Playernum || !ship->alive) {
    DontOwnErr(Playernum, Governor, *shipno);
    return;
  }
  if (ship->type == ShipType::STYPE_POD) {
    g.out << "You cannot change the ownership of spore pods.\n";
    return;
  }

  if ((ship->popn + ship->troops) && !race.God) {
    g.out << "You can't give this ship away while it has crew/mil on board.\n";
    return;
  }
  if (ship->ships && !race.God) {
    g.out
        << "You can't give away this ship, it has other ships loaded on it.\n";
    return;
  }
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
        return;
      }
      break;
    default:
      if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1],
                  APcount)) {
        return;
      }
      break;
  }

  ship->owner = who;
  ship->governor = 0; /* give to the leader */
  capture_stuff(*ship, g);

  putship(&*ship);

  /* set inhabited/explored bits */
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      break;
    case ScopeLevel::LEVEL_STAR:
      stars[ship->storbits] = getstar(ship->storbits);
      setbit(stars[ship->storbits].explored, who);
      putstar(stars[ship->storbits], ship->storbits);
      break;
    case ScopeLevel::LEVEL_PLAN: {
      stars[ship->storbits] = getstar(ship->storbits);
      setbit(stars[ship->storbits].explored, who);
      putstar(stars[ship->storbits], ship->storbits);

      auto planet = getplanet((int)ship->storbits, (int)ship->pnumorbits);
      planet.info[who - 1].explored = 1;
      putplanet(planet, stars[ship->storbits], (int)ship->pnumorbits);

    } break;
    default:
      g.out << "Something wrong with this ship's scope.\n";
      return;
  }

  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
      return;
    default:
      deductAPs(g, APcount, g.snum);
      break;
  }
  g.out << "Owner changed.\n";
  std::string givemsg =
      std::format("{} [{}] gave you {} at {}.\n", race.name, Playernum,
                  ship_to_string(*ship), prin_ship_orbits(*ship));
  warn(who, 0, givemsg);

  if (!race.God) {
    std::string postmsg = std::format("{} [{}] gives {} [{}] a ship.\n",
                                      race.name, Playernum, alien.name, who);
    post(postmsg, NewsType::TRANSFER);
  }
}
}  // namespace GB::commands
