// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void launch(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  Ship *s;
  shipnum_t shipno;

  if (argv.size() < 2) {
    g.out << "Launch what?\n";
    return;
  }

  auto nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        authorized(Governor, *s)) {
      if (!speed_rating(*s) && landed(*s)) {
        g.out << "That ship is not designed to be launched.\n";
        free(s);
        continue;
      }

      if (!s->docked && s->whatorbits != ScopeLevel::LEVEL_SHIP) {
        g.out << std::format("{} is not landed or docked.\n",
                             ship_to_string(*s));
        free(s);
        continue;
      }
      if (!landed(*s)) APcount = 0;
      if (landed(*s) && s->resource > max_resource(*s)) {
        g.out << std::format("{} is too overloaded to launch.\n",
                             ship_to_string(*s));
        free(s);
        continue;
      }
      if (s->whatorbits == ScopeLevel::LEVEL_SHIP) {
        /* Factories cannot be launched once turned on. Maarten */
        if (s->type == ShipType::OTYPE_FACTORY && s->on) {
          g.out << "Factories cannot be launched once turned on.\n";
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
          g.out << std::format(
              "Landed on {}/{}.\n", stars[s->storbits].get_name(),
              stars[s->storbits].get_planet_name(s->pnumorbits));
          putship(*s);
          putship(*s2);
        } else if (s2->whatorbits == ScopeLevel::LEVEL_PLAN) {
          remove_sh_ship(*s, *s2);
          g.out << std::format("{} launched from {}.\n", ship_to_string(*s),
                               ship_to_string(*s2));
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
          g.out << std::format(
              "Orbiting {}/{}.\n", stars[s->storbits].get_name(),
              stars[s->storbits].get_planet_name(s->pnumorbits));
          putship(*s);
          putship(*s2);
        } else if (s2->whatorbits == ScopeLevel::LEVEL_STAR) {
          remove_sh_ship(*s, *s2);
          g.out << std::format("{} launched from {}.\n", ship_to_string(*s),
                               ship_to_string(*s2));
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
          g.out << std::format("Orbiting {}.\n", stars[s->storbits].get_name());
          putship(*s);
          putship(*s2);
        } else if (s2->whatorbits == ScopeLevel::LEVEL_UNIV) {
          remove_sh_ship(*s, *s2);
          g.out << std::format("{} launched from {}.\n", ship_to_string(*s),
                               ship_to_string(*s2));
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
          putship(*s);
          putship(*s2);
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
          if (!enufAP(Playernum, Governor, stars[s->storbits].AP(Playernum - 1),
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
        g.out << std::format("{} undocked from {}.\n", ship_to_string(*s),
                             ship_to_string(*s2));
        putship(*s);
        putship(*s2);
        free(s);
      } else {
        if (!enufAP(Playernum, Governor, stars[s->storbits].AP(Playernum - 1),
                    APcount)) {
          free(s);
          return;
        }
        deductAPs(g, APcount, s->storbits);

        /* adjust x,ypos to absolute coords */
        auto p = getplanet((int)s->storbits, (int)s->pnumorbits);
        g.out << std::format("Planet /{}/{} has gravity field of {:.2f}\n",
                             stars[s->storbits].get_name(),
                             stars[s->storbits].get_planet_name(s->pnumorbits),
                             p.gravity());
        s->xpos =
            stars[s->storbits].xpos() + p.xpos() +
            (double)int_rand((int)(-DIST_TO_LAND / 4), (int)(DIST_TO_LAND / 4));
        s->ypos =
            stars[s->storbits].ypos() + p.ypos() +
            (double)int_rand((int)(-DIST_TO_LAND / 4), (int)(DIST_TO_LAND / 4));

        /* subtract fuel from ship */
        auto fuel = p.gravity() * s->mass * LAUNCH_GRAV_MASS_FACTOR;
        if (s->fuel < fuel) {
          g.out << std::format("{} does not have enough fuel! ({:.1f})\n",
                               ship_to_string(*s), fuel);
          free(s);
          return;
        }
        use_fuel(*s, fuel);
        s->docked = 0;
        s->whatdest = ScopeLevel::LEVEL_UNIV; /* no destination */
        switch (s->type) {
          case ShipType::OTYPE_CANIST:
          case ShipType::OTYPE_GREEN:
            s->special = TimerData{.count = 0};
            break;
          default:
            break;
        }
        s->notified = 0;
        putship(*s);
        if (!p.explored()) {
          /* not yet explored by owner; space exploration causes the
             player to see a whole map */
          p.explored() = 1;
          putplanet(p, stars[s->storbits], s->pnumorbits);
        }
        std::string observed =
            std::format("{} observed launching from planet /{}/{}.\n",
                        ship_to_string(*s), stars[s->storbits].get_name(),
                        stars[s->storbits].get_planet_name(s->pnumorbits));
        for (player_t i = 1; i <= Num_races; i++)
          if (p.info(i - 1).numsectsowned && i != Playernum)
            notify(i, stars[s->storbits].governor(i - 1), observed);

        g.out << std::format("{} launched from planet,", ship_to_string(*s));
        g.out << std::format(" using {:.1f} fuel.\n", fuel);

        switch (s->type) {
          case ShipType::OTYPE_CANIST:
            g.out << "A cloud of dust envelopes your planet.\n";
            break;
          case ShipType::OTYPE_GREEN:
            g.out << "Greenhouse gases surround the planet.\n";
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
