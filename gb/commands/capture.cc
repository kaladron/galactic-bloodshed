// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/capture.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/defense.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/getplace.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

void capture(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const int APcount = 1;
  Ship *ship;
  Ship s;
  player_t oldowner;
  governor_t oldgov;
  int shipdam = 0;
  int booby = 0;
  shipnum_t shipno;
  shipnum_t nextshipno;
  int x = -1;
  int y = -1;
  int what;
  population_t olddpopn;
  population_t olddtroops;
  population_t casualties = 0;
  population_t casualties1 = 0;
  population_t casualties2 = 0;
  population_t casualty_scale = 0;
  double astrength;
  double dstrength;

  if (argv.size() < 2) {
    g.out << "Capture what?\n";
    return;
  }
  if (Governor && Stars[g.snum]->governor[Playernum - 1] != Governor) {
    g.out << "You are not authorized in this system.\n";
    return;
  }
  nextshipno = start_shiplist(g, argv[1]);
  while ((shipno = do_shiplist(&ship, &nextshipno)))
    if (ship->owner != Playernum &&
        in_list(ship->owner, argv[1], *ship, &nextshipno)) {
      if (!landed(*ship)) {
        sprintf(buf, "%s #%ld is not landed on a planet.\n",
                Shipnames[ship->type], shipno);
        notify(Playernum, Governor, buf);
        free(ship);
        continue;
      }
      if (ship->type == ShipType::OTYPE_VN) {
        g.out << "You can't capture Von Neumann machines.\n";
        free(ship);
        continue;
      }
      if (!enufAP(Playernum, Governor, Stars[ship->storbits]->AP[Playernum - 1],
                  APcount)) {
        free(ship);
        continue;
      }

      x = ship->land_x;
      y = ship->land_y;

      auto p = getplanet(ship->storbits, ship->pnumorbits);
      auto sect = getsector(p, x, y);

      if (sect.owner != Playernum) {
        sprintf(buf,
                "You don't own the sector where the ship is landed [%d].\n",
                sect.owner);
        notify(Playernum, Governor, buf);
        free(ship);
        continue;
      }

      if (argv.size() < 4)
        what = CIV;
      else if (argv[3] == "civilians")
        what = CIV;
      else if (argv[3] == "military")
        what = MIL;
      else {
        g.out << "Capture with what?\n";
        free(ship);
        continue;
      }

      population_t boarders;
      if (argv.size() < 3) {
        if (what == CIV)
          boarders = sect.popn;
        else  // MIL
          boarders = sect.troops;
      } else
        boarders = std::stoul(argv[2]);

      if (boarders == 0) {
        g.out << "Illegal number of boarders.\n";
        free(ship);
        continue;
      }

      if ((boarders > sect.popn) && what == CIV)
        boarders = sect.popn;
      else if ((boarders > sect.troops) && what == MIL)
        boarders = sect.troops;

      auto &race = races[Playernum - 1];
      auto &alien = races[ship->owner - 1];

      if (isset(race.allied, (ship->owner))) {
        sprintf(buf, "Boarding the ship of your ally, %s\n", alien.name);
        notify(Playernum, Governor, buf);
      }

      olddpopn = ship->popn;
      olddtroops = ship->troops;
      oldowner = ship->owner;
      oldgov = ship->governor;
      bcopy(ship, &s, sizeof(Ship));

      shipdam = 0;
      casualties = 0;
      casualties1 = 0;
      casualties2 = 0;

      if (what == CIV)
        sect.popn -= boarders;
      else if (what == MIL)
        sect.troops -= boarders;

      if (olddpopn + olddtroops) {
        sprintf(
            buf, "Attack strength: %.2f     Defense strength: %.2f\n",
            astrength = (double)boarders *
                        (what == MIL ? (double)race.fighters * 10.0 : 1.0) *
                        .01 * race.tech * (race.likes[sect.condition] + 0.01) *
                        ((double)Defensedata[sect.condition] + 1.0) *
                        morale_factor((double)(race.morale - alien.morale)),
            dstrength = ((double)ship->popn +
                         (double)ship->troops * 10.0 * (double)alien.fighters) *
                        .01 * alien.tech * ((double)(armor(*ship)) + 0.01) *
                        .01 * (100.0 - (double)ship->damage) *
                        morale_factor((double)(alien.morale - race.morale)));
        notify(Playernum, Governor, buf);
        casualty_scale = std::min(boarders, ship->popn + ship->troops);
        if (astrength > 0.0)
          casualties =
              int_rand(0, round_rand((double)casualty_scale *
                                     (dstrength + 1.0) / (astrength + 1.0)));

        if (dstrength > 0.0) {
          casualties1 =
              int_rand(0, round_rand((double)casualty_scale *
                                     (astrength + 1.0) / (dstrength + 1.0)));
          casualties2 =
              int_rand(0, round_rand((double)casualty_scale *
                                     (astrength + 1.0) / (dstrength + 1.0)));
          shipdam = int_rand(
              0, round_rand(25. * (astrength + 1.0) / (dstrength + 1.0)));
          ship->damage = std::min(100, ship->damage + shipdam);
        }

        casualties = std::min(boarders, casualties);
        boarders -= casualties;

        casualties1 = std::min(olddpopn, casualties1);
        ship->popn -= casualties1;
        ship->mass -= casualties1 * alien.mass;

        casualties2 = std::min(olddtroops, casualties2);
        ship->troops -= casualties2;
        ship->mass -= casualties2 * alien.mass;

      } else if (ship->destruct) { /* booby trapped robot ships */
        booby = int_rand(0, 10 * ship->destruct);
        booby = std::min(100, booby);
        casualties = casualties2 = 0;
        for (unsigned long i = 0; i < boarders; i++)
          casualties += (int_rand(1, 100) < booby);
        boarders -= casualties;
        shipdam += booby;
        ship->damage += booby;
      }
      shipdam = std::min(100, shipdam);
      if (ship->damage >= 100) kill_ship(Playernum, ship);

      if (!(ship->popn + ship->troops) && ship->alive) {
        /* we got 'em */
        ship->owner = Playernum;
        ship->governor = Governor;
        if (what == CIV) {
          ship->popn = std::min(boarders, max_crew(*ship));
          sect.popn += boarders - ship->popn;
          ship->mass += ship->popn * race.mass;
        } else if (what == MIL) {
          ship->troops = std::min(boarders, max_mil(*ship));
          sect.troops += boarders - ship->troops;
          ship->mass += ship->troops * race.mass;
        }
        if (olddpopn + olddtroops && ship->type != ShipType::OTYPE_FACTORY)
          adjust_morale(race, alien, ship->build_cost);
        /* unoccupied ships and factories don't count */
      } else { /* retreat */
        if (what == CIV)
          sect.popn += boarders;
        else if (what == MIL)
          sect.troops += boarders;
      }

      if (!(sect.popn + sect.troops)) sect.owner = 0;

      sprintf(buf, "BULLETIN from %s/%s!!\n", Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      strcpy(telegram_buf, buf);
      sprintf(
          buf, "You are being attacked by%s Player #%d (%s)!!!\n",
          (isset(alien.allied, Playernum)
               ? " your ally"
               : (isset(alien.atwar, Playernum) ? " your enemy" : " neutral")),
          Playernum, race.name);
      strcat(telegram_buf, buf);
      sprintf(buf, "%s at sector %d,%d [owner %d] !\n",
              ship_to_string(*ship).c_str(), x, y, sect.owner);
      strcat(telegram_buf, buf);

      if (booby) {
        sprintf(buf, "Booby trap triggered causing %d%% damage.\n", booby);
        strcat(telegram_buf, buf);
        notify(Playernum, Governor, buf);
      }

      if (shipdam) {
        sprintf(buf, "Total damage: %d%% (now %d%%)\n", shipdam, ship->damage);
        strcat(telegram_buf, buf);
        sprintf(buf, "Damage inflicted:  Them: %d%% (now %d%%)\n", shipdam,
                ship->damage);
        notify(Playernum, Governor, buf);
      }

      if (!ship->alive) {
        sprintf(buf, "              YOUR SHIP WAS DESTROYED!!!\n");
        strcat(telegram_buf, buf);
        g.out << "              Their ship DESTROYED!!!\n";
        sprintf(short_buf, "%s: %s [%d] DESTROYED %s\n", Dispshiploc(ship),
                race.name, Playernum, ship_to_string(s).c_str());
      }

      if (ship->owner == Playernum) {
        sprintf(buf, "%s CAPTURED!\n", ship_to_string(s).c_str());
        notify(oldowner, oldgov, buf);
        sprintf(buf, "VICTORY! The ship is yours!\n");
        notify(Playernum, Governor, buf);
        if (what == CIV)
          sprintf(buf, "%lu boarders move in.\n",
                  std::min(boarders, ship->popn));
        else if (what == MIL)
          sprintf(buf, "%lu troops move in.\n",
                  std::min(boarders, ship->troops));
        notify(Playernum, Governor, buf);
        capture_stuff(*ship, g);
        sprintf(short_buf, "%s: %s [%d] CAPTURED %s\n", Dispshiploc(ship),
                race.name, Playernum, ship_to_string(s).c_str());
      } else if (ship->popn + ship->troops) {
        sprintf(buf, "You fought them off!\n");
        notify(oldowner, oldgov, buf);
        g.out << "The boarding was repulsed; try again.\n";
        sprintf(short_buf, "%s: %s [%d] assaults %s\n", Dispshiploc(ship),
                race.name, Playernum, ship_to_string(s).c_str());
      }
      if (ship->alive) {
        if (sect.popn + sect.troops + boarders) {
          sprintf(buf, "You killed all the aliens in this sector!\n");
          strcat(telegram_buf, buf);
          p.info[Playernum - 1].mob_points -= sect.mobilization;
        }
        if (!boarders) {
          g.out << "Oh no! They killed your party to the last man!\n";
        }
      } else {
        sprintf(buf, "Your ship was weakened too much!\n");
        strcat(telegram_buf, buf);
        g.out << "The assault weakened their ship too much!\n";
      }

      if (casualties || casualties1 || casualties2) {
        sprintf(buf, "Casualties: Yours: %ld civ/%ld mil, Theirs: %ld %s\n",
                casualties1, casualties2, casualties,
                what == CIV ? "civ" : "mil");
        strcat(telegram_buf, buf);
        sprintf(buf, "Casualties: Yours: %ld %s, Theirs: %ld civ/%ld mil\n",
                casualties, what == CIV ? "civ" : "mil", casualties1,
                casualties2);
        notify(Playernum, Governor, buf);
      }
      warn(oldowner, oldgov, telegram_buf);
      if (ship->owner != oldowner || !ship->alive) post(short_buf, COMBAT);
      notify_star(Playernum, Governor, ship->storbits, short_buf);
      putship(ship);
      putsector(sect, p, x, y);
      putplanet(p, Stars[g.snum], g.pnum);
      putrace(race);
      putrace(alien);
      deductAPs(Playernum, Governor, APcount, ship->storbits, 0);
      free(ship);
    } else
      free(ship);
}
