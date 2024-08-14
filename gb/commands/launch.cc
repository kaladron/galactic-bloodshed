// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"

module commands;

namespace GB::commands {
void launch(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  Ship *s;
  shipnum_t shipno;
  shipnum_t nextshipno;
  double fuel;

  if (argv.size() < 2) {
    g.out << "Launch what?\n";
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        authorized(Governor, *s)) {
      if (!speed_rating(*s) && landed(*s)) {
        sprintf(buf, "That ship is not designed to be launched.\n");
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }

      if (!(s->docked || s->whatorbits == ScopeLevel::LEVEL_SHIP)) {
        sprintf(buf, "%s is not landed or docked.\n",
                ship_to_string(*s).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      if (!landed(*s)) APcount = 0;
      if (landed(*s) && s->resource > max_resource(*s)) {
        sprintf(buf, "%s is too overloaded to launch.\n",
                ship_to_string(*s).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      if (s->whatorbits == ScopeLevel::LEVEL_SHIP) {
        /* Factories cannot be launched once turned on. Maarten */
        if (s->type == ShipType::OTYPE_FACTORY && s->on) {
          notify(Playernum, Governor,
                 "Factories cannot be launched once turned on.\n");
          g.out << "Consider using 'scrap'.\n";
          free(s);
          continue;
        }
        auto s2 = getship(s->destshipno);
        if (landed(*s2)) {
          remove_sh_ship(*s, *s2);
          auto p = getplanet(s2->storbits, s2->pnumorbits);
          insert_sh_plan(p, s);
          putplanet(p, stars[s2->storbits], s2->pnumorbits);
          s->storbits = s2->storbits;
          s->pnumorbits = s2->pnumorbits;
          s->destpnum = s2->pnumorbits;
          s->deststar = s2->deststar;
          s->xpos = s2->xpos;
          s->ypos = s2->ypos;
          s->land_x = s2->land_x;
          s->land_y = s2->land_y;
          s->docked = 1;
          s->whatdest = ScopeLevel::LEVEL_PLAN;
          s2->mass -= s->mass;
          s2->hanger -= size(*s);
          sprintf(buf, "Landed on %s/%s.\n", stars[s->storbits].name,
                  stars[s->storbits].pnames[s->pnumorbits]);
          notify(Playernum, Governor, buf);
          putship(s);
          putship(&*s2);
        } else if (s2->whatorbits == ScopeLevel::LEVEL_PLAN) {
          remove_sh_ship(*s, *s2);
          sprintf(buf, "%s launched from %s.\n", ship_to_string(*s).c_str(),
                  ship_to_string(*s2).c_str());
          notify(Playernum, Governor, buf);
          s->xpos = s2->xpos;
          s->ypos = s2->ypos;
          s->docked = 0;
          s->whatdest = ScopeLevel::LEVEL_UNIV;
          s2->mass -= s->mass;
          s2->hanger -= size(*s);
          auto p = getplanet(s2->storbits, s2->pnumorbits);
          insert_sh_plan(p, s);
          s->storbits = s2->storbits;
          s->pnumorbits = s2->pnumorbits;
          putplanet(p, stars[s2->storbits], s2->pnumorbits);
          sprintf(buf, "Orbiting %s/%s.\n", stars[s->storbits].name,
                  stars[s->storbits].pnames[s->pnumorbits]);
          notify(Playernum, Governor, buf);
          putship(s);
          putship(&*s2);
        } else if (s2->whatorbits == ScopeLevel::LEVEL_STAR) {
          remove_sh_ship(*s, *s2);
          sprintf(buf, "%s launched from %s.\n", ship_to_string(*s).c_str(),
                  ship_to_string(*s2).c_str());
          notify(Playernum, Governor, buf);
          s->xpos = s2->xpos;
          s->ypos = s2->ypos;
          s->docked = 0;
          s->whatdest = ScopeLevel::LEVEL_UNIV;
          s2->mass -= s->mass;
          s2->hanger -= size(*s);
          stars[s2->storbits] = getstar(s2->storbits);
          insert_sh_star(stars[s2->storbits], s);
          s->storbits = s2->storbits;
          putstar(stars[s2->storbits], s2->storbits);
          sprintf(buf, "Orbiting %s.\n", stars[s->storbits].name);
          notify(Playernum, Governor, buf);
          putship(s);
          putship(&*s2);
        } else if (s2->whatorbits == ScopeLevel::LEVEL_UNIV) {
          remove_sh_ship(*s, *s2);
          sprintf(buf, "%s launched from %s.\n", ship_to_string(*s).c_str(),
                  ship_to_string(*s2).c_str());
          notify(Playernum, Governor, buf);
          s->xpos = s2->xpos;
          s->ypos = s2->ypos;
          s->docked = 0;
          s->whatdest = ScopeLevel::LEVEL_UNIV;
          s2->mass -= s->mass;
          s2->hanger -= size(*s);
          getsdata(&Sdata);
          insert_sh_univ(&Sdata, s);
          g.out << "Universe level.\n";
          putsdata(&Sdata);
          putship(s);
          putship(&*s2);
        } else {
          g.out << "You can't launch that ship.\n";
          free(s);
          continue;
        }
        free(s);
      } else if (s->whatdest == ScopeLevel::LEVEL_SHIP) {
        auto s2 = getship(s->destshipno);
        if (s2->whatorbits == ScopeLevel::LEVEL_UNIV) {
          if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
            free(s);
            continue;
          }
          deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
        } else {
          if (!enufAP(Playernum, Governor, stars[s->storbits].AP[Playernum - 1],
                      APcount)) {
            free(s);
            continue;
          }
          deductAPs(g, APcount, s->storbits);
        }
        s->docked = 0;
        s->whatdest = ScopeLevel::LEVEL_UNIV;
        s->destshipno = 0;
        s2->docked = 0;
        s2->whatdest = ScopeLevel::LEVEL_UNIV;
        s2->destshipno = 0;
        sprintf(buf, "%s undocked from %s.\n", ship_to_string(*s).c_str(),
                ship_to_string(*s2).c_str());
        notify(Playernum, Governor, buf);
        putship(s);
        putship(&*s2);
        free(s);
      } else {
        if (!enufAP(Playernum, Governor, stars[s->storbits].AP[Playernum - 1],
                    APcount)) {
          free(s);
          return;
        }
        deductAPs(g, APcount, s->storbits);

        /* adjust x,ypos to absolute coords */
        auto p = getplanet((int)s->storbits, (int)s->pnumorbits);
        sprintf(buf, "Planet /%s/%s has gravity field of %.2f\n",
                stars[s->storbits].name,
                stars[s->storbits].pnames[s->pnumorbits], p.gravity());
        notify(Playernum, Governor, buf);
        s->xpos =
            stars[s->storbits].xpos + p.xpos +
            (double)int_rand((int)(-DIST_TO_LAND / 4), (int)(DIST_TO_LAND / 4));
        s->ypos =
            stars[s->storbits].ypos + p.ypos +
            (double)int_rand((int)(-DIST_TO_LAND / 4), (int)(DIST_TO_LAND / 4));

        /* subtract fuel from ship */
        fuel = p.gravity() * s->mass * LAUNCH_GRAV_MASS_FACTOR;
        if (s->fuel < fuel) {
          sprintf(buf, "%s does not have enough fuel! (%.1f)\n",
                  ship_to_string(*s).c_str(), fuel);
          notify(Playernum, Governor, buf);
          free(s);
          return;
        }
        use_fuel(*s, fuel);
        s->docked = 0;
        s->whatdest = ScopeLevel::LEVEL_UNIV; /* no destination */
        switch (s->type) {
          case ShipType::OTYPE_CANIST:
          case ShipType::OTYPE_GREEN:
            s->special.timer.count = 0;
            break;
          default:
            break;
        }
        s->notified = 0;
        putship(s);
        if (!p.explored) {
          /* not yet explored by owner; space exploration causes the
             player to see a whole map */
          p.explored = 1;
          putplanet(p, stars[s->storbits], s->pnumorbits);
        }
        sprintf(buf, "%s observed launching from planet /%s/%s.\n",
                ship_to_string(*s).c_str(), stars[s->storbits].name,
                stars[s->storbits].pnames[s->pnumorbits]);
        for (player_t i = 1; i <= Num_races; i++)
          if (p.info[i - 1].numsectsowned && i != Playernum)
            notify(i, stars[s->storbits].governor[i - 1], buf);

        sprintf(buf, "%s launched from planet,", ship_to_string(*s).c_str());
        notify(Playernum, Governor, buf);
        sprintf(buf, " using %.1f fuel.\n", fuel);
        notify(Playernum, Governor, buf);

        switch (s->type) {
          case ShipType::OTYPE_CANIST:
            notify(Playernum, Governor,
                   "A cloud of dust envelopes your planet.\n");
            break;
          case ShipType::OTYPE_GREEN:
            notify(Playernum, Governor,
                   "Green house gases surround the planet.\n");
            break;
          default:
            break;
        }
        free(s);
      }
    } else
      free(s);
}
}  // namespace GB::commands
