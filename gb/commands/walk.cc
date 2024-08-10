// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/fire.h"
#include "gb/load.h"
#include "gb/move.h"
#include "gb/tele.h"

module commands;

namespace GB::commands {
void walk(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const ap_t APcount = 1;
  int x;
  int y;
  int i;
  int succ = 0;
  int civ;
  int mil;
  player_t oldowner;
  governor_t oldgov;
  int strength;
  int strength1;

  if (argv.size() < 2) {
    g.out << "Walk what?\n";
    return;
  }
  auto ship = getship(argv[1]);
  if (!ship) {
    g.out << "No such ship.\n";
    return;
  }
  if (testship(*ship, Playernum, Governor)) {
    g.out << "You do not control this ship.\n";
    return;
  }
  if (ship->type != ShipType::OTYPE_AFV) {
    g.out << "This ship doesn't walk!\n";
    return;
  }
  if (!landed(*ship)) {
    g.out << "This ship is not landed on a planet.\n";
    return;
  }
  if (!ship->popn) {
    g.out << "No crew.\n";
    return;
  }
  if (ship->fuel < AFV_FUEL_COST) {
    sprintf(buf, "You don't have %.1f fuel to move it.\n", AFV_FUEL_COST);
    notify(Playernum, Governor, buf);
    return;
  }
  if (!enufAP(Playernum, Governor, stars[ship->storbits].AP[Playernum - 1],
              APcount)) {
    return;
  }
  auto p = getplanet((int)ship->storbits, (int)ship->pnumorbits);
  auto &race = races[Playernum - 1];

  if (!get_move(argv[2][0], (int)ship->land_x, (int)ship->land_y, &x, &y, p)) {
    g.out << "Illegal move.\n";
    return;
  }
  if (x < 0 || y < 0 || x > p.Maxx - 1 || y > p.Maxy - 1) {
    sprintf(buf, "Illegal coordinates %d,%d.\n", x, y);
    notify(Playernum, Governor, buf);
    putplanet(p, stars[g.snum], g.pnum);
    return;
  }
  /* check to see if player is permited on the sector type */
  auto sect = getsector(p, x, y);
  if (!race.likes[sect.condition]) {
    notify(Playernum, Governor,
           "Your ships cannot walk into that sector type!\n");
    return;
  }
  /* if the sector is occupied by non-aligned AFVs, each one will attack */
  Shiplist shiplist{p.ships};
  for (auto ship2 : shiplist) {
    if (ship2.owner != Playernum && ship2.type == ShipType::OTYPE_AFV &&
        landed(ship2) && retal_strength(ship2) && (ship2.land_x == x) &&
        (ship2.land_y == y)) {
      auto &alien = races[ship2.owner - 1];
      if (!isset(race.allied, ship2.owner) || !isset(alien.allied, Playernum)) {
        while ((strength = retal_strength(ship2)) &&
               (strength1 = retal_strength(*ship))) {
          use_destruct(ship2, strength);
          notify(Playernum, Governor, long_buf);
          warn(ship2.owner, ship2.governor, long_buf);
          if (!ship2.alive) post(short_buf, NewsType::COMBAT);
          notify_star(Playernum, Governor, ship->storbits, short_buf);
          if (strength1) {
            use_destruct(*ship, strength1);
            notify(Playernum, Governor, long_buf);
            warn(ship2.owner, ship2.governor, long_buf);
            if (!ship2.alive) post(short_buf, NewsType::COMBAT);
            notify_star(Playernum, Governor, ship->storbits, short_buf);
          }
        }
        putship(&ship2);
      }
    }
    if (!ship->alive) break;
  }
  /* if the sector is occupied by non-aligned player, attack them first */
  if (ship->popn && ship->alive && sect.owner && sect.owner != Playernum) {
    oldowner = sect.owner;
    oldgov = stars[ship->storbits].governor[sect.owner - 1];
    auto &alien = races[oldowner - 1];
    if (!isset(race.allied, oldowner) || !isset(alien.allied, Playernum)) {
      if (!retal_strength(*ship)) {
        g.out << "You have nothing to attack with!\n";
        return;
      }
      while ((sect.popn + sect.troops) && retal_strength(*ship)) {
        civ = (int)sect.popn;
        mil = (int)sect.troops;
        mech_attack_people(&*ship, &civ, &mil, race, alien, sect, x, y, 0,
                           long_buf, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(alien.Playernum, oldgov, long_buf);
        notify_star(Playernum, Governor, ship->storbits, short_buf);
        post(short_buf, NewsType::COMBAT);

        people_attack_mech(&*ship, sect.popn, sect.troops, alien, race, sect, x,
                           y, long_buf, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(alien.Playernum, oldgov, long_buf);
        notify_star(Playernum, Governor, ship->storbits, short_buf);
        if (!ship->alive) post(short_buf, NewsType::COMBAT);

        sect.popn = civ;
        sect.troops = mil;
        if (!(sect.popn + sect.troops)) {
          p.info[sect.owner - 1].mob_points -= (int)sect.mobilization;
          sect.owner = 0;
        }
      }
    }
    putrace(alien);
    putrace(race);
    putplanet(p, stars[g.snum], g.pnum);
    putsector(sect, p, x, y);
  }

  if ((sect.owner == Playernum || isset(race.allied, sect.owner) ||
       !sect.owner) &&
      ship->alive)
    succ = 1;

  if (ship->alive && ship->popn && succ) {
    sprintf(buf, "%s moving from %d,%d to %d,%d on %s.\n",
            ship_to_string(*ship).c_str(), (int)ship->land_x, (int)ship->land_y,
            x, y, dispshiploc(*ship).c_str());
    ship->land_x = x;
    ship->land_y = y;
    use_fuel(*ship, AFV_FUEL_COST);
    for (i = 1; i <= Num_races; i++)
      if (i != Playernum && p.info[i - 1].numsectsowned)
        notify(i, stars[g.snum].governor[i - 1], buf);
  }
  putship(&*ship);
  deductAPs(g, APcount, ship->storbits);
}
}  // namespace GB::commands
