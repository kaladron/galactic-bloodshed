// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/shootblast.h"
module commands;

namespace {

/**
 * @brief Land a friendly ship onto another ship or planet.
 *
 * This function allows a friendly ship to land onto another ship or planet.
 * It performs various checks to ensure the legality of the landing operation.
 * If the landing is successful, the ship is loaded onto the target ship and
 * the necessary updates are made to the game state.
 *
 * @param argv The command arguments.
 * @param g The GameObj representing the game state.
 * @param s A Ship object representing the ship to be landed.
 */
void land_friendly(const command_t &argv, GameObj &g, Ship &s) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  double fuel;
  double Dist;

  auto ship2tmp = string_to_shipnum(argv[2]);
  if (!ship2tmp) {
    g.out << std::format("Ship {} wasn't found.\n", argv[2]);
    return;
  }
  auto s2 = getship(*ship2tmp);
  if (!s2) {
    g.out << std::format("Ship #{} wasn't found.\n", *ship2tmp);
    return;
  }
  auto ship2no = *ship2tmp;
  if (testship(*s2, Playernum, Governor)) {
    g.out << "Illegal format.\n";
    return;
  }
  if (s2->type == ShipType::OTYPE_FACTORY) {
    g.out << "Can't land on factories.\n";
    return;
  }
  if (landed(s)) {
    if (!landed(*s2)) {
      g.out << std::format("{} is not landed on a planet.\n",
                           ship_to_string(*s2));
      return;
    }
    if (s2->storbits != s.storbits) {
      g.out << "These ships are not in the same star system.\n";
      return;
    }
    if (s2->pnumorbits != s.pnumorbits) {
      g.out << "These ships are not landed on the same planet.\n";
      return;
    }
    if ((s2->land_x != s.land_x) || (s2->land_y != s.land_y)) {
      g.out << "These ships are not in the same sector.\n";
      return;
    }
    if (s.on) {
      g.out << std::format("{} must be turned off before loading.\n",
                           ship_to_string(s));
      return;
    }
    if (size(s) > hanger(*s2)) {
      g.out << std::format(
          "Mothership does not have {} hanger space available to load ship.\n",
          size(s));
      return;
    }
    /* ok, load 'em up */
    remove_sh_plan(s);
    /* get the target ship again because it had a pointer changed (and put to
     * disk) in the remove routines */
    s2 = getship(ship2no);
    insert_sh_ship(&s, &*s2);
    /* increase mass of mothership */
    s2->mass += s.mass;
    s2->hanger += size(s);
    fuel = 0.0;
    g.out << std::format("{} loaded onto {} using {} fuel.\n",
                         ship_to_string(s), ship_to_string(*s2), fuel);
    s.docked = 1;
    putship(&*s2);
  } else if (s.docked) {
    g.out << std::format("{} is already docked or landed.\n",
                         ship_to_string(s));
    return;
  } else {
    /* Check if the ships are in the same scope level. Maarten */
    if (s.whatorbits != s2->whatorbits) {
      g.out << "Those ships are not in the same scope.\n";
      return;
    }

    /* check to see if close enough to land */
    Dist = sqrt((double)Distsq(s2->xpos, s2->ypos, s.xpos, s.ypos));
    if (Dist > DIST_TO_DOCK) {
      g.out << std::format("{} must be {} or closer to {}.\n",
                           ship_to_string(s), DIST_TO_DOCK,
                           ship_to_string(*s2));
      return;
    }
    fuel = 0.05 + Dist * 0.025 * sqrt(s.mass);
    if (s.fuel < fuel) {
      g.out << "Not enough fuel.\n";
      return;
    }
    if (size(s) > hanger(*s2)) {
      g.out << std::format(
          "Mothership does not have {} hanger space available to load ship.\n",
          size(s));
      return;
    }
    use_fuel(s, fuel);

    /* remove the ship from whatever scope it is currently in */
    if (s.whatorbits == ScopeLevel::LEVEL_PLAN)
      remove_sh_plan(s);
    else if (s.whatorbits == ScopeLevel::LEVEL_STAR)
      remove_sh_star(s);
    else {
      g.out << "Ship is not in planet or star scope.\n";
      return;
    }
    /* get the target ship again because it had a pointer changed (and put to
     * disk) in the remove routines */
    s2 = getship(ship2no);
    insert_sh_ship(&s, &*s2);
    /* increase mass of mothership */
    s2->mass += s.mass;
    s2->hanger += size(s);
    g.out << std::format("{} landed on {} using {} fuel.\n", ship_to_string(s),
                         ship_to_string(*s2), fuel);
    s.docked = 1;
    putship(&*s2);
  }
}
}  // namespace

namespace GB::commands {
void land(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  Ship *s;

  shipnum_t shipno;
  int x = -1;
  int y = -1;
  int i;
  int numdest;
  int strength;
  int damage;
  double fuel;
  double Dist;
  shipnum_t nextshipno;

  numdest = 0;  // TODO(jeffbailey): Init to zero.

  if (argv.size() < 2) {
    g.out << "Land what?\n";
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        authorized(Governor, *s)) {
      if (overloaded(*s)) {
        sprintf(buf, "%s is too overloaded to land.\n",
                ship_to_string(*s).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      if (s->type == ShipType::OTYPE_QUARRY) {
        g.out << "You can't load quarries onto ship.\n";
        free(s);
        continue;
      }
      if (docked(*s)) {
        g.out << "That ship is docked to another ship.\n";
        free(s);
        continue;
      }

      /* attempting to land on a friendly ship (for carriers/stations/etc) */
      if (argv[2][0] == '#') {
        land_friendly(argv, g, *s);
        free(s);
        continue;
      } else { /* attempting to land on a planet */
        if (s->docked) {
          sprintf(buf, "%s is docked.\n", ship_to_string(*s).c_str());
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        sscanf(argv[2].c_str(), "%d,%d", &x, &y);
        if (s->whatorbits != ScopeLevel::LEVEL_PLAN) {
          sprintf(buf, "%s doesn't orbit a planet.\n",
                  ship_to_string(*s).c_str());
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (!Shipdata[s->type][ABIL_CANLAND]) {
          sprintf(buf, "This ship is not equipped to land.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if ((s->storbits != g.snum) || (s->pnumorbits != g.pnum)) {
          sprintf(buf, "You have to cs to the planet it orbits.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (!speed_rating(*s)) {
          sprintf(buf, "This ship is not rated for maneuvering.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (!enufAP(Playernum, Governor, stars[s->storbits].AP[Playernum - 1],
                    APcount)) {
          free(s);
          continue;
        }

        auto p = getplanet(s->storbits, s->pnumorbits);

        sprintf(buf, "Planet /%s/%s has gravity field of %.2f.\n",
                stars[s->storbits].name,
                stars[s->storbits].pnames[s->pnumorbits], p.gravity());
        notify(Playernum, Governor, buf);

        sprintf(buf, "Distance to planet: %.2f.\n",
                Dist = sqrt((double)Distsq(stars[s->storbits].xpos + p.xpos,
                                           stars[s->storbits].ypos + p.ypos,
                                           s->xpos, s->ypos)));
        notify(Playernum, Governor, buf);

        if (Dist > DIST_TO_LAND) {
          sprintf(buf, "%s must be %.3g or closer to the planet (%.2f).\n",
                  ship_to_string(*s).c_str(), DIST_TO_LAND, Dist);
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }

        fuel = s->mass * p.gravity() * LAND_GRAV_MASS_FACTOR;

        if ((x < 0) || (y < 0) || (x > p.Maxx - 1) || (y > p.Maxy - 1)) {
          sprintf(buf, "Illegal coordinates.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }

        if (DEFENSE) {
          /* people who have declared war on you will fire at your landing ship
           */
          for (i = 1; i <= Num_races; i++)
            if (s->alive && i != Playernum && p.info[i - 1].popn &&
                p.info[i - 1].guns && p.info[i - 1].destruct) {
              auto &alien = races[i - 1];
              if (isset(alien.atwar, s->owner)) {
                /* attack the landing ship */
                strength =
                    MIN((int)p.info[i - 1].guns, (int)p.info[i - 1].destruct);
                if (strength) {
                  damage =
                      shoot_planet_to_ship(alien, s, strength, buf, short_buf);
                  post(short_buf, NewsType::COMBAT);
                  notify_star(0, 0, s->storbits, short_buf);
                  warn(i, stars[s->storbits].governor[i - 1], buf);
                  notify(s->owner, s->governor, buf);
                  p.info[i - 1].destruct -= strength;
                }
              }
            }
          if (!s->alive) {
            putplanet(p, stars[s->storbits], s->pnumorbits);
            putship(s);
            free(s);
            continue;
          }
        }
        /* check to see if the ship crashes from lack of fuel or damage */
        if (auto [did_crash, roll] = crash(*s, fuel); did_crash) {
          /* damaged ships stand of chance of crash landing */
          auto smap = getsmap(p);
          numdest = shoot_ship_to_planet(
              s, p, round_rand((double)(s->destruct) / 3.), x, y, smap, 0,
              GTYPE_HEAVY, long_buf, short_buf);
          putsmap(smap, p);
          sprintf(
              buf,
              "BOOM!! %s crashes on sector %d,%d with blast radius of %d.\n",
              ship_to_string(*s).c_str(), x, y, numdest);
          for (i = 1; i <= Num_races; i++)
            if (p.info[i - 1].numsectsowned || i == Playernum)
              warn(i, stars[s->storbits].governor[i - 1], buf);
          if (roll)
            sprintf(buf, "Ship damage %d%% (you rolled a %d)\n", (int)s->damage,
                    roll);
          else
            sprintf(buf, "You had %.1ff while the landing required %.1ff\n",
                    s->fuel, fuel);
          notify(Playernum, Governor, buf);
          kill_ship((int)s->owner, s);
        } else {
          s->land_x = x;
          s->land_y = y;
          s->xpos = p.xpos + stars[s->storbits].xpos;
          s->ypos = p.ypos + stars[s->storbits].ypos;
          use_fuel(*s, fuel);
          s->docked = 1;
          s->whatdest = ScopeLevel::LEVEL_PLAN; /* no destination */
          s->deststar = s->storbits;
          s->destpnum = s->pnumorbits;
        }

        auto sect = getsector(p, x, y);

        if (sect.condition == SectorType::SEC_WASTED) {
          sprintf(buf, "Warning: That sector is a wasteland!\n");
          notify(Playernum, Governor, buf);
        } else if (sect.owner && sect.owner != Playernum) {
          auto &race = races[Playernum - 1];
          auto &alien = races[sect.owner - 1];
          if (!(isset(race.allied, sect.owner) &&
                isset(alien.allied, Playernum))) {
            sprintf(buf, "You have landed on an alien sector (%s).\n",
                    alien.name);
            notify(Playernum, Governor, buf);
          } else {
            sprintf(buf, "You have landed on allied sector (%s).\n",
                    alien.name);
            notify(Playernum, Governor, buf);
          }
        }
        if (s->whatorbits == ScopeLevel::LEVEL_UNIV)
          deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
        else
          deductAPs(g, APcount, s->storbits);

        putplanet(p, stars[s->storbits], s->pnumorbits);

        if (numdest) putsector(sect, p, x, y);

        /* send messages to anyone there */
        sprintf(buf, "%s observed landing on sector %d,%d,planet /%s/%s.\n",
                ship_to_string(*s).c_str(), s->land_x, s->land_y,
                stars[s->storbits].name,
                stars[s->storbits].pnames[s->pnumorbits]);
        for (i = 1; i <= Num_races; i++)
          if (p.info[i - 1].numsectsowned && i != Playernum)
            notify(i, stars[s->storbits].governor[i - 1], buf);

        sprintf(buf, "%s landed on planet.\n", ship_to_string(*s).c_str());
        notify(Playernum, Governor, buf);
      }
      putship(s);
      free(s);
    } else
      free(s);
}
}  // namespace GB::commands
