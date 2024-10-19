// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace {
int jettison_check(GameObj &g, int amt, int max) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (amt == 0) amt = max;
  if (amt < 0) {
    g.out << "Nice try.\n";
    return -1;
  }
  if (amt > max) {
    sprintf(buf, "You can jettison at most %d\n", max);
    notify(Playernum, Governor, buf);
    return -1;
  }
  return amt;
}
}  // namespace

namespace GB::commands {
void jettison(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 0;
  int Mod = 0;
  shipnum_t shipno;
  shipnum_t nextshipno;
  int amt;
  char commod;
  Ship *s;

  if (argv.size() < 2) {
    g.out << "Jettison what?\n";
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        authorized(Governor, *s)) {
      if (s->owner != Playernum || !s->alive) {
        free(s);
        continue;
      }
      if (landed(*s)) {
        g.out << "Ship is landed, cannot jettison.\n";
        free(s);
        continue;
      }
      if (!s->active) {
        sprintf(buf, "%s is irradiated and inactive.\n",
                ship_to_string(*s).c_str());
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      if (s->whatorbits == ScopeLevel::LEVEL_UNIV) {
        if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
          free(s);
          continue;
        }
      } else if (!enufAP(Playernum, Governor,
                         stars[s->storbits].AP(Playernum - 1), APcount)) {
        free(s);
        continue;
      }

      if (argv.size() > 3)
        amt = std::stoi(argv[3]);
      else
        amt = 0;

      auto &race = races[Playernum - 1];

      commod = argv[2][0];
      switch (commod) {
        case 'x':
          if ((amt = jettison_check(g, amt, (int)(s->crystals))) > 0) {
            s->crystals -= amt;
            sprintf(buf, "%d crystal%s jettisoned.\n", amt,
                    (amt == 1) ? "" : "s");
            notify(Playernum, Governor, buf);
            Mod = 1;
          }
          break;
        case 'c':
          if ((amt = jettison_check(g, amt, (int)(s->popn))) > 0) {
            s->popn -= amt;
            s->mass -= amt * race.mass;
            sprintf(buf, "%d crew %s into deep space.\n", amt,
                    (amt == 1) ? "hurls itself" : "hurl themselves");
            notify(Playernum, Governor, buf);
            sprintf(buf, "Complement of %s is now %lu.\n",
                    ship_to_string(*s).c_str(), s->popn);
            notify(Playernum, Governor, buf);
            Mod = 1;
          }
          break;
        case 'm':
          if ((amt = jettison_check(g, amt, (int)(s->troops))) > 0) {
            sprintf(buf, "%d military %s into deep space.\n", amt,
                    (amt == 1) ? "hurls itself" : "hurl themselves");
            notify(Playernum, Governor, buf);
            sprintf(buf, "Complement of ship #%lu is now %lu.\n", shipno,
                    s->troops - amt);
            notify(Playernum, Governor, buf);
            s->troops -= amt;
            s->mass -= amt * race.mass;
            Mod = 1;
          }
          break;
        case 'd':
          if ((amt = jettison_check(g, amt, (int)(s->destruct))) > 0) {
            use_destruct(*s, amt);
            sprintf(buf, "%d destruct jettisoned.\n", amt);
            notify(Playernum, Governor, buf);
            if (!max_crew(*s)) {
              sprintf(buf, "\n%s ", ship_to_string(*s).c_str());
              notify(Playernum, Governor, buf);
              if (s->destruct) {
                g.out << "still boobytrapped.\n";
              } else {
                g.out << "no longer boobytrapped.\n";
              }
            }
            Mod = 1;
          }
          break;
        case 'f':
          if ((amt = jettison_check(g, amt, (int)(s->fuel))) > 0) {
            use_fuel(*s, (double)amt);
            sprintf(buf, "%d fuel jettisoned.\n", amt);
            notify(Playernum, Governor, buf);
            Mod = 1;
          }
          break;
        case 'r':
          if ((amt = jettison_check(g, amt, (int)(s->resource))) > 0) {
            use_resource(*s, amt);
            sprintf(buf, "%d resources jettisoned.\n", amt);
            notify(Playernum, Governor, buf);
            Mod = 1;
          }
          break;
        default:
          g.out << "No such commodity valid.\n";
          return;
      }
      if (Mod) putship(s);
      free(s);
    } else
      free(s);
}
}  // namespace GB::commands
