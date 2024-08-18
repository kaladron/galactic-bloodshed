// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  dock.c -- dock a ship and..... assault -- a very un-PC version of dock */

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/buffers.h"

module commands;

namespace GB::commands {
void dock(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = (argv[0] == "dock") ? 0 : 1;
  int Assault = (argv[0] == "assault") ? 1 : 0;
  Ship *s;
  Ship ship;
  population_t boarders = 0;
  int dam = 0;
  int dam2 = 0;
  int booby = 0;
  int what;
  shipnum_t ship2no;
  shipnum_t shipno;
  shipnum_t nextshipno;
  player_t old2owner;
  governor_t old2gov;
  population_t casualties = 0;
  population_t casualties2 = 0;
  population_t casualties3 = 0;
  int casualty_scale = 0;
  double fuel;
  double bstrength;
  double b2strength;
  double Dist;
  Race *race;
  Race *alien;

  if (argv.size() < 3) {
    g.out << "Dock with what?\n";
    return;
  }
  if (argv.size() < 5)
    what = MIL;
  else if (Assault) {
    if (argv[4] == "civilians")
      what = CIV;
    else if (argv[4] == "military")
      what = MIL;
    else {
      g.out << "Assault with what?\n";
      return;
    }
  }

  nextshipno = start_shiplist(g, argv[1]);
  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        (!Governor || s->governor == Governor)) {
      if (Assault && s->type == ShipType::STYPE_POD) {
        g.out << "Sorry. Pods cannot be used to assault.\n";
        free(s);
        continue;
      }
      if (!Assault) {
        if (s->docked || s->whatorbits == ScopeLevel::LEVEL_SHIP) {
          sprintf(buf, "%s is already docked.\n", ship_to_string(*s).c_str());
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
      } else if (s->docked) {
        g.out << "Your ship is already docked.\n";
        free(s);
        continue;
      } else if (s->whatorbits == ScopeLevel::LEVEL_SHIP) {
        g.out << "Your ship is landed on another ship.\n";
        free(s);
        continue;
      }

      if (s->whatorbits == ScopeLevel::LEVEL_UNIV) {
        if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
          free(s);
          continue;
        }
      } else if (!enufAP(Playernum, Governor,
                         stars[s->storbits].AP[Playernum - 1], APcount)) {
        free(s);
        continue;
      }

      if (Assault && (what == CIV) && !s->popn) {
        notify(Playernum, Governor,
               "You have no crew on this ship to assault with.\n");
        free(s);
        continue;
      }
      if (Assault && (what == MIL) && !s->troops) {
        notify(Playernum, Governor,
               "You have no troops on this ship to assault with.\n");
        free(s);
        continue;
      }

      auto shiptmp = string_to_shipnum(argv[2]);
      if (!shiptmp) {
        g.out << "Invalid ship number.\n";
      }
      ship2no = *shiptmp;

      if (shipno == ship2no) {
        g.out << "You can't dock with yourself!\n";
        free(s);
        continue;
      }

      auto s2 = getship(ship2no);
      if (!s2) {
        g.out << "The ship wasn't found.\n";
        free(s);
        return;
      }

      if (!Assault && testship(*s2, Playernum, Governor)) {
        g.out << "You are not authorized to do this.\n";
        free(s);
        return;
      }

      /* Check if ships are on same scope level. Maarten */
      if (s->whatorbits != s2->whatorbits) {
        g.out << "Those ships are not in the same scope.\n";
        free(s);
        return;
      }

      if (Assault && (s2->type == ShipType::OTYPE_VN)) {
        notify(Playernum, Governor,
               "You can't assault Von Neumann machines.\n");
        free(s);
        return;
      }

      if (s2->docked || (s->whatorbits == ScopeLevel::LEVEL_SHIP)) {
        sprintf(buf, "%s is already docked.\n", ship_to_string(*s2).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        return;
      }

      Dist = sqrt((double)Distsq(s2->xpos, s2->ypos, s->xpos, s->ypos));
      fuel =
          0.05 + Dist * 0.025 * (Assault ? 2.0 : 1.0) * sqrt((double)s->mass);

      if (Dist > DIST_TO_DOCK) {
        sprintf(buf, "%s must be %.2f or closer to %s.\n",
                ship_to_string(*s).c_str(), DIST_TO_DOCK,
                ship_to_string(*s2).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      if (s->docked && Assault) {
        /* first undock the target ship */
        s->docked = 0;
        s->whatdest = ScopeLevel::LEVEL_UNIV;
        auto s3 = getship(s->destshipno);
        s3->docked = 0;
        s3->whatdest = ScopeLevel::LEVEL_UNIV;
        putship(&*s3);
      }

      if (fuel > s->fuel) {
        sprintf(buf, "Not enough fuel.\n");
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      sprintf(buf, "Distance to %s: %.2f.\n", ship_to_string(*s2).c_str(),
              Dist);
      notify(Playernum, Governor, buf);
      sprintf(buf, "This maneuver will take %.2f fuel (of %.2f.)\n\n", fuel,
              s->fuel);
      notify(Playernum, Governor, buf);

      if (s2->docked && !Assault) {
        sprintf(buf, "%s is already docked.\n", ship_to_string(*s2).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        return;
      }
      /* defending fire gets defensive fire */
      bcopy(&*s2, &ship, sizeof(Ship)); /* for reports */
      if (Assault) {
        // Set the command to be distinctive here.  In the target function,
        // APcount is set to 0 and cew is set to 3.
        command_t fire_argv{"fire-from-dock", std::format("#{0}", ship2no),
                            std::format("#{0}", shipno)};
        GB::commands::fire(fire_argv, g);
        /* retrieve ships again, since battle may change ship stats */
        free(s);
        (void)getship(&s, shipno);
        s2 = getship(ship2no);
        if (!s->alive) {
          free(s);
          continue;
        }
        if (!s2->alive) {
          free(s);
          return;
        }
      }

      if (Assault) {
        alien = &races[s2->owner - 1];
        race = &races[Playernum - 1];
        if (argv.size() >= 4) {
          sscanf(argv[3].c_str(), "%lu", &boarders);
          if ((what == MIL) && (boarders > s->troops))
            boarders = s->troops;
          else if ((what == CIV) && (boarders > s->popn))
            boarders = s->popn;
        } else {
          if (what == CIV)
            boarders = s->popn;
          else if (what == MIL)
            boarders = s->troops;
        }
        if (boarders > s2->max_crew) boarders = s2->max_crew;

        /* Allow assault of crewless ships. */
        if (s2->max_crew && boarders <= 0) {
          sprintf(buf, "Illegal number of boarders (%lu).\n", boarders);
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        old2owner = s2->owner;
        old2gov = s2->governor;
        if (what == MIL)
          s->troops -= boarders;
        else if (what == CIV)
          s->popn -= boarders;
        s->mass -= boarders * race->mass;
        sprintf(
            buf, "Boarding strength :%.2f       Defense strength: %.2f.\n",
            bstrength = boarders * (what == MIL ? 10 * race->fighters : 1) *
                        .01 * race->tech *
                        morale_factor((double)(race->morale - alien->morale)),
            b2strength = (s2->popn + 10 * s2->troops * alien->fighters) * .01 *
                         alien->tech *
                         morale_factor((double)(alien->morale - race->morale)));
        notify(Playernum, Governor, buf);
      }

      /* the ship moves into position, regardless of success of attack */
      use_fuel(*s, fuel);
      s->xpos = s2->xpos + int_rand(-1, 1);
      s->ypos = s2->ypos + int_rand(-1, 1);
      if (s->hyper_drive.on) {
        s->hyper_drive.on = 0;
        g.out << "Hyper-drive deactivated.\n";
      }
      if (Assault) {
        /* if the assaulted ship is docked, undock it first */
        if (s2->docked && s2->whatdest == ScopeLevel::LEVEL_SHIP) {
          auto s3 = getship(s2->destshipno);
          s3->docked = 0;
          s3->whatdest = ScopeLevel::LEVEL_UNIV;
          s3->destshipno = 0;
          putship(&*s3);

          s2->docked = 0;
          s2->whatdest = ScopeLevel::LEVEL_UNIV;
          s2->destshipno = 0;
        }
        /* nuke both populations, ships */
        casualty_scale = MIN(boarders, s2->troops + s2->popn);

        if (b2strength) { /* otherwise the ship surrenders */
          casualties =
              int_rand(0, round_rand((double)casualty_scale *
                                     (b2strength + 1.0) / (bstrength + 1.0)));
          casualties = MIN(boarders, casualties);
          boarders -= casualties;

          dam = int_rand(
              0, round_rand(25. * (b2strength + 1.0) / (bstrength + 1.0)));
          dam = MIN(100, dam);
          s->damage = MIN(100, s->damage + dam);
          if (s->damage >= 100) kill_ship(Playernum, s);

          casualties2 =
              int_rand(0, round_rand((double)casualty_scale *
                                     (bstrength + 1.0) / (b2strength + 1.0)));
          casualties2 = MIN(s2->popn, casualties2);
          casualties3 =
              int_rand(0, round_rand((double)casualty_scale *
                                     (bstrength + 1.0) / (b2strength + 1.0)));
          casualties3 = MIN(s2->troops, casualties3);
          s2->popn -= casualties2;
          s2->mass -= casualties2 * alien->mass;
          s2->troops -= casualties3;
          s2->mass -= casualties3 * alien->mass;
          /* (their mass) */
          dam2 = int_rand(
              0, round_rand(25. * (bstrength + 1.0) / (b2strength + 1.0)));
          dam2 = MIN(100, dam2);
          s2->damage = MIN(100, s2->damage + dam2);
          if (s2->damage >= 100) kill_ship(Playernum, &*s2);
        } else {
          s2->popn = 0;
          s2->troops = 0;
          booby = 0;
          /* do booby traps */
          /* check for boobytrapping */
          if (!s2->max_crew && s2->destruct)
            booby = int_rand(0, 10 * (int)s2->destruct);
          booby = MIN(100, booby);
        }

        if ((!s2->popn && !s2->troops) && s->alive && s2->alive) {
          /* we got 'em */
          s->docked = 1;
          s->whatdest = ScopeLevel::LEVEL_SHIP;
          s->destshipno = ship2no;

          s2->docked = 1;
          s2->whatdest = ScopeLevel::LEVEL_SHIP;
          s2->destshipno = shipno;
          old2owner = s2->owner;
          old2gov = s2->governor;
          s2->owner = s->owner;
          s2->governor = s->governor;
          if (what == MIL)
            s2->troops = boarders;
          else
            s2->popn = boarders;
          s2->mass += boarders * race->mass; /* our mass */
          if (casualties2 + casualties3) {
            /* You must kill to get morale */
            adjust_morale(*race, *alien, (int)s2->build_cost);
          }
        } else { /* retreat */
          if (what == MIL)
            s->troops += boarders;
          else if (what == CIV)
            s->popn += boarders;
          s->mass += boarders * race->mass;
          adjust_morale(*alien, *race, (int)race->fighters);
        }

        /* races find out about each other */
        alien->translate[Playernum - 1] =
            MIN(alien->translate[Playernum - 1] + 5, 100);
        race->translate[old2owner - 1] =
            MIN(race->translate[old2owner - 1] + 5, 100);

        if (!boarders && (s2->popn + s2->troops)) /* boarding party killed */
          alien->translate[Playernum - 1] =
              MIN(alien->translate[Playernum - 1] + 25, 100);
        if (s2->owner == Playernum) /* captured ship */
          race->translate[old2owner - 1] =
              MIN(race->translate[old2owner - 1] + 25, 100);
        putrace(*race);
        putrace(*alien);
      } else {
        s->docked = 1;
        s->whatdest = ScopeLevel::LEVEL_SHIP;
        s->destshipno = ship2no;

        s2->docked = 1;
        s2->whatdest = ScopeLevel::LEVEL_SHIP;
        s2->destshipno = shipno;
      }

      if (Assault) {
        sprintf(telegram_buf, "%s ASSAULTED by %s at %s\n",
                ship_to_string(ship).c_str(), ship_to_string(*s).c_str(),
                prin_ship_orbits(*s2).c_str());
        sprintf(buf, "Your damage: %d%%, theirs: %d%%.\n", dam2, dam);
        strcat(telegram_buf, buf);
        if (!s2->max_crew && s2->destruct) {
          sprintf(buf, "(Your boobytrap gave them %d%% damage.)\n", booby);
          strcat(telegram_buf, buf);
          sprintf(buf, "Their boobytrap gave you %d%% damage!)\n", booby);
          notify(Playernum, Governor, buf);
        }
        sprintf(buf, "Damage taken:  You: %d%% (now %d%%)\n", dam, s->damage);
        notify(Playernum, Governor, buf);
        if (!s->alive) {
          sprintf(buf, "              YOUR SHIP WAS DESTROYED!!!\n");
          notify(Playernum, Governor, buf);
          sprintf(buf, "              Their ship DESTROYED!!!\n");
          strcat(telegram_buf, buf);
        }
        sprintf(buf, "              Them: %d%% (now %d%%)\n", dam2, s2->damage);
        notify(Playernum, Governor, buf);
        if (!s2->alive) {
          sprintf(
              buf,
              "              Their ship DESTROYED!!!  Boarders are dead.\n");
          notify(Playernum, Governor, buf);
          sprintf(buf, "              YOUR SHIP WAS DESTROYED!!!\n");
          strcat(telegram_buf, buf);
        }
        if (s->alive) {
          if (s2->owner == Playernum) {
            sprintf(buf, "CAPTURED!\n");
            strcat(telegram_buf, buf);
            sprintf(buf, "VICTORY! the ship is yours!\n");
            notify(Playernum, Governor, buf);
            if (boarders) {
              sprintf(buf, "%lu boarders move in.\n", boarders);
              notify(Playernum, Governor, buf);
            }
            capture_stuff(*s2, g);
          } else if (s2->popn + s2->troops) {
            sprintf(buf, "The boarding was repulsed; try again.\n");
            notify(Playernum, Governor, buf);
            sprintf(buf, "You fought them off!\n");
            strcat(telegram_buf, buf);
          }
        } else {
          sprintf(buf, "The assault was too much for your bucket of bolts.\n");
          notify(Playernum, Governor, buf);
          sprintf(buf, "The assault was too much for their ship..\n");
          strcat(telegram_buf, buf);
        }
        if (s2->alive) {
          if (s2->max_crew && !boarders) {
            sprintf(
                buf,
                "Oh no! They killed your boarding party to the last man!\n");
            notify(Playernum, Governor, buf);
          }
          if (!s->popn && !s->troops) {
            sprintf(buf, "You killed all their crew!\n");
            strcat(telegram_buf, buf);
          }
        } else {
          sprintf(buf, "The assault weakened their ship too much!\n");
          notify(Playernum, Governor, buf);
          sprintf(buf, "Your ship was weakened too much!\n");
          strcat(telegram_buf, buf);
        }
        sprintf(buf, "Casualties: Yours: %lu mil/%lu civ    Theirs: %lu %s\n",
                casualties3, casualties2, casualties,
                what == MIL ? "mil" : "civ");
        strcat(telegram_buf, buf);
        sprintf(
            buf, "Crew casualties: Yours: %lu %s    Theirs: %lu mil/%lu civ\n",
            casualties, what == MIL ? "mil" : "civ", casualties3, casualties2);
        notify(Playernum, Governor, buf);
        warn(old2owner, old2gov, telegram_buf);
        sprintf(buf, "%s %s %s at %s.\n", ship_to_string(*s).c_str(),
                s2->alive ? (s2->owner == Playernum ? "CAPTURED" : "assaulted")
                          : "DESTROYED",
                ship_to_string(ship).c_str(), prin_ship_orbits(*s).c_str());
        if (s2->owner == Playernum || !s2->alive) post(buf, NewsType::COMBAT);
        notify_star(Playernum, Governor, s->storbits, buf);
      } else {
        sprintf(buf, "%s docked with %s.\n", ship_to_string(*s).c_str(),
                ship_to_string(*s2).c_str());
        notify(Playernum, Governor, buf);
      }

      if (g.level == ScopeLevel::LEVEL_UNIV)
        deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
      else
        deductAPs(g, APcount, g.snum);

      s->notified = s2->notified = 0;
      putship(s);
      putship(&*s2);
      free(s);
    } else
      free(s);
}
}  // namespace GB::commands
