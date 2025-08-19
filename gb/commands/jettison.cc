// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

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
    notify(Playernum, Governor,
           std::format("You can jettison at most {}\n", max));
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
        notify(Playernum, Governor,
               std::format("{} is irradiated and inactive.\n",
                           ship_to_string(*s)));
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
            notify(Playernum, Governor,
                   std::format("{} crystal{} jettisoned.\n", amt,
                               (amt == 1) ? "" : "s"));
            Mod = 1;
          }
          break;
        case 'c':
          if ((amt = jettison_check(g, amt, (int)(s->popn))) > 0) {
            s->popn -= amt;
            s->mass -= amt * race.mass;
            notify(
                Playernum, Governor,
                std::format("{} crew {} into deep space.\n", amt,
                            (amt == 1) ? "hurls itself" : "hurl themselves"));
            notify(Playernum, Governor,
                   std::format("Complement of {} is now {}.\n",
                               ship_to_string(*s), s->popn));
            Mod = 1;
          }
          break;
        case 'm':
          if ((amt = jettison_check(g, amt, (int)(s->troops))) > 0) {
            notify(
                Playernum, Governor,
                std::format("{} military {} into deep space.\n", amt,
                            (amt == 1) ? "hurls itself" : "hurl themselves"));
            notify(Playernum, Governor,
                   std::format("Complement of ship #{} is now {}.\n", shipno,
                               s->troops - amt));
            s->troops -= amt;
            s->mass -= amt * race.mass;
            Mod = 1;
          }
          break;
        case 'd':
          if ((amt = jettison_check(g, amt, (int)(s->destruct))) > 0) {
            use_destruct(*s, amt);
            notify(Playernum, Governor,
                   std::format("{} destruct jettisoned.\n", amt));
            if (!max_crew(*s)) {
              notify(Playernum, Governor,
                     std::format("\n{} ", ship_to_string(*s)));
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
            notify(Playernum, Governor,
                   std::format("{} fuel jettisoned.\n", amt));
            Mod = 1;
          }
          break;
        case 'r':
          if ((amt = jettison_check(g, amt, (int)(s->resource))) > 0) {
            use_resource(*s, amt);
            notify(Playernum, Governor,
                   std::format("{} resources jettisoned.\n", amt));
            Mod = 1;
          }
          break;
        default:
          g.out << "No such commodity valid.\n";
          return;
      }
      if (Mod) putship(*s);
      free(s);
    } else
      free(s);
}
}  // namespace GB::commands
