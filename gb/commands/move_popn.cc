// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/defense.h"
#include "gb/fire.h"
#include "gb/move.h"

module commands;

namespace GB::commands {
void move_popn(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int what;
  if (argv[0] == "move") {
    what = CIV;
  } else {
    what = MIL;  // deploy
  }
  int Assault;
  int APcost; /* unfriendly movement */
  int casualties;
  int casualties2;
  int casualties3;

  int people;
  int oldpopn;
  int old2popn;
  int old3popn;
  int x = -1;
  int y = -1;
  int x2 = -1;
  int y2 = -1;
  int old2owner;
  int old2gov;
  int absorbed;
  int n;
  int done;
  double astrength;
  double dstrength;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "Wrong scope\n");
    return;
  }
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet = getplanet(g.snum, g.pnum);

  if (planet.slaved_to > 0 && planet.slaved_to != Playernum) {
    sprintf(buf, "That planet has been enslaved!\n");
    notify(Playernum, Governor, buf);
    return;
  }
  sscanf(argv[1].c_str(), "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet.Maxx - 1 || y > planet.Maxy - 1) {
    sprintf(buf, "Origin coordinates illegal.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  /* movement loop */
  done = 0;
  n = 0;
  while (!done) {
    auto sect = getsector(planet, x, y);
    if (sect.owner != Playernum) {
      sprintf(buf, "You don't own sector %d,%d!\n", x, y);
      notify(Playernum, Governor, buf);
      return;
    }
    if (!get_move(argv[2][n++], x, y, &x2, &y2, planet)) {
      g.out << "Finished.\n";
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (x2 < 0 || y2 < 0 || x2 > planet.Maxx - 1 || y2 > planet.Maxy - 1) {
      sprintf(buf, "Illegal coordinates %d,%d.\n", x2, y2);
      notify(Playernum, Governor, buf);
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (!adjacent(x, y, x2, y2, planet)) {
      sprintf(buf, "Illegal move - to adjacent sectors only!\n");
      notify(Playernum, Governor, buf);
      return;
    }

    /* ok, the move is legal */
    auto sect2 = getsector(planet, x2, y2);
    if (argv.size() >= 4) {
      people = std::stoi(argv[3]);
      if (people < 0) {
        if (what == CIV)
          people = sect.popn + people;
        else if (what == MIL)
          people = sect.troops + people;
      }
    } else {
      if (what == CIV)
        people = sect.popn;
      else if (what == MIL)
        people = sect.troops;
    }

    if ((what == CIV && (abs(people) > sect.popn)) ||
        (what == MIL && (abs(people) > sect.troops)) || people <= 0) {
      if (what == CIV)
        sprintf(buf, "Bad value - %lu civilians in [%d,%d]\n", sect.popn, x, y);
      else if (what == MIL)
        sprintf(buf, "Bad value - %lu troops in [%d,%d]\n", sect.troops, x, y);
      notify(Playernum, Governor, buf);
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    sprintf(buf, "%d %s moved.\n", people,
            what == CIV ? "population" : "troops");
    notify(Playernum, Governor, buf);

    /* check for defending mechs */
    mech_defend(Playernum, Governor, &people, what, planet, x2, y2, sect2);
    if (!people) {
      putsector(sect, planet, x, y);
      putsector(sect2, planet, x2, y2);
      putplanet(planet, stars[g.snum], g.pnum);
      g.out << "Attack aborted.\n";
      return;
    }

    if (sect2.owner && (sect2.owner != Playernum))
      Assault = 1;
    else
      Assault = 0;

    /* action point cost depends on the size of the group being moved */
    if (what == CIV)
      APcost = MOVE_FACTOR * ((int)log(1.0 + (double)people) + Assault) + 1;
    else if (what == MIL)
      APcost = MOVE_FACTOR * ((int)log10(1.0 + (double)people) + Assault) + 1;

    if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcost)) {
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (Assault) {
      ground_assaults[Playernum - 1][sect2.owner - 1][g.snum] += 1;
      auto &race = races[Playernum - 1];
      auto &alien = races[sect2.owner - 1];
      /* races find out about each other */
      alien.translate[Playernum - 1] =
          MIN(alien.translate[Playernum - 1] + 5, 100);
      race.translate[sect2.owner - 1] =
          MIN(race.translate[sect2.owner - 1] + 5, 100);

      old2owner = (int)(sect2.owner);
      old2gov = stars[g.snum].governor[sect2.owner - 1];
      if (what == CIV)
        sect.popn = std::max(0L, sect.popn - people);
      else if (what == MIL)
        sect.troops = std::max(0L, sect.troops - people);

      if (what == CIV)
        sprintf(buf, "%d civ assault %lu civ/%lu mil\n", people, sect2.popn,
                sect2.troops);
      else if (what == MIL)
        sprintf(buf, "%d mil assault %lu civ/%lu mil\n", people, sect2.popn,
                sect2.troops);
      notify(Playernum, Governor, buf);
      oldpopn = people;
      old2popn = sect2.popn;
      old3popn = sect2.troops;

      ground_attack(race, alien, &people, what, &sect2.popn, &sect2.troops,
                    Defensedata[sect.condition], Defensedata[sect2.condition],
                    race.likes[sect.condition], alien.likes[sect2.condition],
                    &astrength, &dstrength, &casualties, &casualties2,
                    &casualties3);

      sprintf(buf, "Attack: %.2f   Defense: %.2f.\n", astrength, dstrength);
      notify(Playernum, Governor, buf);

      if (!(sect2.popn + sect2.troops)) { /* we got 'em */
        sect2.owner = Playernum;
        /* mesomorphs absorb the bodies of their victims */
        absorbed = 0;
        if (race.absorb) {
          absorbed = int_rand(0, old2popn + old3popn);
          sprintf(buf, "%d alien bodies absorbed.\n", absorbed);
          notify(Playernum, Governor, buf);
          sprintf(buf, "Metamorphs have absorbed %d bodies!!!\n", absorbed);
          notify(old2owner, old2gov, buf);
        }
        if (what == CIV)
          sect2.popn = people + absorbed;
        else if (what == MIL) {
          sect2.popn = absorbed;
          sect2.troops = people;
        }
        adjust_morale(race, alien, (int)alien.fighters);
      } else { /* retreat */
        absorbed = 0;
        if (alien.absorb) {
          absorbed = int_rand(0, oldpopn - people);
          sprintf(buf, "%d alien bodies absorbed.\n", absorbed);
          notify(old2owner, old2gov, buf);
          sprintf(buf, "Metamorphs have absorbed %d bodies!!!\n", absorbed);
          notify(Playernum, Governor, buf);
          sect2.popn += absorbed;
        }
        if (what == CIV)
          sect.popn += people;
        else if (what == MIL)
          sect.troops += people;
        adjust_morale(alien, race, (int)race.fighters);
      }

      sprintf(telegram_buf,
              "/%s/%s: %s [%d] %c(%d,%d) assaults %s [%d] %c(%d,%d) %s\n",
              stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
              Playernum, Dessymbols[sect.condition], x, y, alien.name,
              alien.Playernum, Dessymbols[sect2.condition], x2, y2,
              (sect2.owner == Playernum ? "VICTORY" : "DEFEAT"));

      if (sect2.owner == Playernum) {
        sprintf(buf, "VICTORY! The sector is yours!\n");
        notify(Playernum, Governor, buf);
        sprintf(buf, "Sector CAPTURED!\n");
        strcat(telegram_buf, buf);
        if (people) {
          sprintf(buf, "%d %s move in.\n", people,
                  what == CIV ? "civilians" : "troops");
          notify(Playernum, Governor, buf);
        }
        planet.info[Playernum - 1].mob_points += (int)sect2.mobilization;
        planet.info[old2owner - 1].mob_points -= (int)sect2.mobilization;
      } else {
        sprintf(buf, "The invasion was repulsed; try again.\n");
        notify(Playernum, Governor, buf);
        sprintf(buf, "You fought them off!\n");
        strcat(telegram_buf, buf);
        done = 1; /* end loop */
      }

      if (!(sect.popn + sect.troops + people)) {
        sprintf(buf, "You killed all of them!\n");
        strcat(telegram_buf, buf);
        /* increase modifier */
        race.translate[old2owner - 1] =
            MIN(race.translate[old2owner - 1] + 5, 100);
      }
      if (!people) {
        sprintf(buf, "Oh no! They killed your party to the last man!\n");
        notify(Playernum, Governor, buf);
        /* increase modifier */
        alien.translate[Playernum - 1] =
            MIN(alien.translate[Playernum - 1] + 5, 100);
      }
      putrace(alien);
      putrace(race);

      sprintf(buf, "Casualties: You: %d civ/%d mil, Them: %d %s\n", casualties2,
              casualties3, casualties, what == CIV ? "civ" : "mil");
      strcat(telegram_buf, buf);
      warn(old2owner, old2gov, telegram_buf);
      sprintf(buf, "Casualties: You: %d %s, Them: %d civ/%d mil\n", casualties,
              what == CIV ? "civ" : "mil", casualties2, casualties3);
      notify(Playernum, Governor, buf);
    } else {
      if (what == CIV) {
        sect.popn -= people;
        sect2.popn += people;
      } else if (what == MIL) {
        sect.troops -= people;
        sect2.troops += people;
      }
      if (!sect2.owner)
        planet.info[Playernum - 1].mob_points += (int)sect2.mobilization;
      sect2.owner = Playernum;
    }

    if (!(sect.popn + sect.troops)) {
      planet.info[Playernum - 1].mob_points -= (int)sect.mobilization;
      sect.owner = 0;
    }

    if (!(sect2.popn + sect2.troops)) {
      sect2.owner = 0;
      done = 1;
    }

    putsector(sect, planet, x, y);
    putsector(sect2, planet, x2, y2);

    deductAPs(g, APcost, g.snum);
    x = x2;
    y = y2; /* get ready for the next round */
  }
  g.out << "Finished.\n";
}
}  // namespace GB::commands
