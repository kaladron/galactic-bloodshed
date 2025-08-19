// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include <strings.h>

module commands;

namespace GB::commands {
void capture(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const ap_t APcount = 1;
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
  PopulationType what;
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
  if (Governor && stars[g.snum].governor(Playernum - 1) != Governor) {
    g.out << "You are not authorized in this system.\n";
    return;
  }
  nextshipno = start_shiplist(g, argv[1]);
  while ((shipno = do_shiplist(&ship, &nextshipno)))
    if (ship->owner != Playernum &&
        in_list(ship->owner, argv[1], *ship, &nextshipno)) {
      if (!landed(*ship)) {
        notify(Playernum, Governor,
               std::format("{} #{} is not landed on a planet.\n",
                           Shipnames[ship->type], shipno));
        free(ship);
        continue;
      }
      if (ship->type == ShipType::OTYPE_VN) {
        g.out << "You can't capture Von Neumann machines.\n";
        free(ship);
        continue;
      }
      if (!enufAP(Playernum, Governor, stars[ship->storbits].AP(Playernum - 1),
                  APcount)) {
        free(ship);
        continue;
      }

      x = ship->land_x;
      y = ship->land_y;

      auto p = getplanet(ship->storbits, ship->pnumorbits);
      auto sect = getsector(p, x, y);

      if (sect.owner != Playernum) {
        notify(Playernum, Governor,
               std::format(
                   "You don't own the sector where the ship is landed [{}].\n",
                   sect.owner));
        free(ship);
        continue;
      }

      if (argv.size() < 4)
        what = PopulationType::CIV;
      else if (argv[3] == "civilians")
        what = PopulationType::CIV;
      else if (argv[3] == "military")
        what = PopulationType::MIL;
      else {
        g.out << "Capture with what?\n";
        free(ship);
        continue;
      }

      population_t boarders;
      if (argv.size() < 3) {
        if (what == PopulationType::CIV)
          boarders = sect.popn;
        else  // PopulationType::MIL
          boarders = sect.troops;
      } else
        boarders = std::stoul(argv[2]);

      if (boarders == 0) {
        g.out << "Illegal number of boarders.\n";
        free(ship);
        continue;
      }

      if ((boarders > sect.popn) && what == PopulationType::CIV)
        boarders = sect.popn;
      else if ((boarders > sect.troops) && what == PopulationType::MIL)
        boarders = sect.troops;

      auto &race = races[Playernum - 1];
      auto &alien = races[ship->owner - 1];

      if (isset(race.allied, (ship->owner))) {
        notify(Playernum, Governor,
               std::format("Boarding the ship of your ally, {}\n", alien.name));
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

      if (what == PopulationType::CIV)
        sect.popn -= boarders;
      else if (what == PopulationType::MIL)
        sect.troops -= boarders;

      if (olddpopn + olddtroops) {
        notify(
            Playernum, Governor,
            std::format(
                "Attack strength: {:.2f}     Defense strength: {:.2f}\n",
                astrength =
                    (double)boarders *
                    (what == PopulationType::MIL ? (double)race.fighters * 10.0
                                                 : 1.0) *
                    .01 * race.tech * (race.likes[sect.condition] + 0.01) *
                    ((double)Defensedata[sect.condition] + 1.0) *
                    morale_factor((double)(race.morale - alien.morale)),
                dstrength =
                    ((double)ship->popn +
                     (double)ship->troops * 10.0 * (double)alien.fighters) *
                    .01 * alien.tech * ((double)(armor(*ship)) + 0.01) * .01 *
                    (100.0 - (double)ship->damage) *
                    morale_factor((double)(alien.morale - race.morale))));
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
        if (what == PopulationType::CIV) {
          ship->popn = std::min(boarders, max_crew(*ship));
          sect.popn += boarders - ship->popn;
          ship->mass += ship->popn * race.mass;
        } else if (what == PopulationType::MIL) {
          ship->troops = std::min(boarders, max_mil(*ship));
          sect.troops += boarders - ship->troops;
          ship->mass += ship->troops * race.mass;
        }
        if (olddpopn + olddtroops && ship->type != ShipType::OTYPE_FACTORY)
          adjust_morale(race, alien, ship->build_cost);
        /* unoccupied ships and factories don't count */
      } else { /* retreat */
        if (what == PopulationType::CIV)
          sect.popn += boarders;
        else if (what == PopulationType::MIL)
          sect.troops += boarders;
      }

      if (!(sect.popn + sect.troops)) sect.owner = 0;

      std::string telegram = std::format(
          "BULLETIN from {}/{}!!\n", stars[ship->storbits].get_name(),
          stars[ship->storbits].get_planet_name(ship->pnumorbits));
      telegram += std::format(
          "You are being attacked by{} Player #{} ({})!!!\n",
          (isset(alien.allied, Playernum)
               ? " your ally"
               : (isset(alien.atwar, Playernum) ? " your enemy" : " neutral")),
          Playernum, race.name);
      telegram += std::format("{} at sector {},{} [owner {}] !\n",
                              ship_to_string(*ship), x, y, sect.owner);

      if (booby) {
        telegram +=
            std::format("Booby trap triggered causing {}% damage.\n", booby);
        notify(
            Playernum, Governor,
            std::format("Booby trap triggered causing {}% damage.\n", booby));
      }

      if (shipdam) {
        telegram +=
            std::format("Total damage: {}% (now {}%)\n", shipdam, ship->damage);
        notify(Playernum, Governor,
               std::format("Damage inflicted:  Them: {}% (now {}%)\n", shipdam,
                           ship->damage));
      }

      if (!ship->alive) {
        telegram += "              YOUR SHIP WAS DESTROYED!!!\n";
        g.out << "              Their ship DESTROYED!!!\n";
        auto short_buf =
            std::format("{}: {} [{}] DESTROYED {}\n", dispshiploc(*ship),
                        race.name, Playernum, ship_to_string(s));
      }

      if (ship->owner == Playernum) {
        notify(oldowner, oldgov,
               std::format("{} CAPTURED!\n", ship_to_string(s)));
        notify(Playernum, Governor, "VICTORY! The ship is yours!\n");
        if (what == PopulationType::CIV)
          notify(Playernum, Governor,
                 std::format("{} boarders move in.\n",
                             std::min(boarders, ship->popn)));
        else if (what == PopulationType::MIL)
          notify(Playernum, Governor,
                 std::format("{} troops move in.\n",
                             std::min(boarders, ship->troops)));
        capture_stuff(*ship, g);
        auto short_buf =
            std::format("{}: {} [{}] CAPTURED {}\n", dispshiploc(*ship),
                        race.name, Playernum, ship_to_string(s));
      } else if (ship->popn + ship->troops) {
        notify(oldowner, oldgov, "You fought them off!\n");
        g.out << "The boarding was repulsed; try again.\n";
        auto short_buf =
            std::format("{}: {} [{}] assaults {}\n", dispshiploc(*ship),
                        race.name, Playernum, ship_to_string(s));
      }
      if (ship->alive) {
        if (sect.popn + sect.troops + boarders) {
          telegram += "You killed all the aliens in this sector!\n";
          p.info[Playernum - 1].mob_points -= sect.mobilization;
        }
        if (!boarders) {
          g.out << "Oh no! They killed your party to the last man!\n";
        }
      } else {
        telegram += "Your ship was weakened too much!\n";
        g.out << "The assault weakened their ship too much!\n";
      }

      if (casualties || casualties1 || casualties2) {
        telegram +=
            std::format("Casualties: Yours: {} civ/{} mil, Theirs: {} {}\n",
                        casualties1, casualties2, casualties,
                        what == PopulationType::CIV ? "civ" : "mil");
        notify(
            Playernum, Governor,
            std::format("Casualties: Yours: {} {}, Theirs: {} civ/{} mil\n",
                        casualties, what == PopulationType::CIV ? "civ" : "mil",
                        casualties1, casualties2));
      }
      warn(oldowner, oldgov, telegram);
      if (ship->owner != oldowner || !ship->alive) {
        auto short_msg = std::format(
            "{}: {} [{}] {} {}\n", dispshiploc(*ship), race.name, Playernum,
            (ship->alive ? "assaults" : "DESTROYED"), ship_to_string(s));
        post(short_msg, NewsType::COMBAT);
        notify_star(Playernum, Governor, ship->storbits, short_msg);
      }
      putship(*ship);
      putsector(sect, p, x, y);
      putplanet(p, stars[g.snum], g.pnum);
      putrace(race);
      putrace(alien);
      deductAPs(g, APcount, ship->storbits);
      free(ship);
    } else
      free(ship);
}
}  // namespace GB::commands
