// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
/*! Ship vs planet */
void bombard(const command_t &argv, GameObj &g) {
  int Playernum = g.player;
  int Governor = g.governor;
  ap_t APcount = 1;
  shipnum_t fromship;
  shipnum_t nextshipno;
  Ship *from;
  int x;
  int y;

  /* for telegramming and retaliating */
  Nuked.fill(0);

  if (argv.size() < 2) {
    g.out << "Syntax: 'bombard <ship> [<x,y> [<strength>]]'.\n";
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, argv[1], *from, &nextshipno) &&
        authorized(Governor, *from)) {
      if (!from->active) {
        g.out << std::format("{} is irradiated and inactive.\n",
                             ship_to_string(*from));
        free(from);
        continue;
      }

      if (from->whatorbits != ScopeLevel::LEVEL_PLAN) {
        g.out << "You must be in orbit around a planet to bombard.\n";
        free(from);
        continue;
      }
      if (from->type == ShipType::OTYPE_AFV && !landed(*from)) {
        g.out << "This ship is not landed on the planet.\n";
        free(from);
        continue;
      }
      if (!enufAP(Playernum, Governor, stars[from->storbits].AP(Playernum - 1),
                  APcount)) {
        free(from);
        continue;
      }

      auto maxstrength = check_retal_strength(*from);

      int strength =
          (argv.size() > 3) ? std::stoi(argv[3]) : check_retal_strength(*from);

      if (strength > maxstrength) {
        strength = maxstrength;
        g.out << std::format("{} set to {}\n",
                             laser_on(*from) ? "Laser strength" : "Guns",
                             strength);
      }

      /* check to see if there is crystal overload */
      if (laser_on(*from)) check_overload(*from, 0, &strength);

      if (strength <= 0) {
        g.out << "No attack.\n";
        putship(*from);
        free(from);
        continue;
      }

      /* get planet */
      auto p = getplanet(from->storbits, from->pnumorbits);

      if (argv.size() > 2) {
        sscanf(argv[2].c_str(), "%d,%d", &x, &y);
        if (x < 0 || x > p.Maxx - 1 || y < 0 || y > p.Maxy - 1) {
          g.out << "Illegal sector.\n";
          free(from);
          continue;
        }
      } else {
        x = int_rand(0, (int)p.Maxx - 1);
        y = int_rand(0, (int)p.Maxy - 1);
      }
      if (landed(*from) && !adjacent(p, {from->land_x, from->land_y}, {x, y})) {
        g.out << "You are not adjacent to that sector.\n";
        free(from);
        continue;
      }

      bool has_defense = has_planet_defense(p.ships, Playernum);

      if (has_defense && !landed(*from)) {
        g.out << "Target has planetary defense networks.\n";
        g.out << "These have to be eliminated before you can attack sectors.\n";
        free(from);
        continue;
      }

      auto smap = getsmap(p);
      char long_buf[1024], short_buf[256];
      auto numdest = shoot_ship_to_planet(*from, p, strength, x, y, smap, 0, 0,
                                          long_buf, short_buf);
      putsmap(smap, p);

      if (numdest < 0) {
        g.out << "Illegal attack.\n";
        free(from);
        continue;
      }

      if (laser_on(*from))
        use_fuel(*from, 2.0 * (double)strength);
      else
        use_destruct(*from, strength);

      post(short_buf, NewsType::COMBAT);
      notify_star(Playernum, Governor, from->storbits, short_buf);
      for (auto i = 1; i <= Num_races; i++)
        if (Nuked[i - 1])
          warn(i, stars[from->storbits].governor(i - 1), long_buf);
      notify(Playernum, Governor, long_buf);

      if (DEFENSE) {
        /* planet retaliates - AFVs are immune to this */
        if (numdest && from->type != ShipType::OTYPE_AFV) {
          for (auto i = 1; i <= Num_races; i++)
            if (Nuked[i - 1] && !p.slaved_to) {
              /* add planet defense strength */
              auto &alien = races[i - 1];
              strength = MIN(p.info[i - 1].destruct, p.info[i - 1].guns);

              p.info[i - 1].destruct -= strength;

              shoot_planet_to_ship(alien, *from, strength, long_buf, short_buf);
              warn(i, stars[from->storbits].governor(i - 1), long_buf);
              notify(Playernum, Governor, long_buf);
              if (!from->alive) post(short_buf, NewsType::COMBAT);
              notify_star(Playernum, Governor, from->storbits, short_buf);
            }
        }
      }

      /* protecting ships retaliate individually if damage was inflicted */
      /* AFVs are immune to this */
      if (numdest && from->alive && from->type != ShipType::OTYPE_AFV) {
        Shiplist shiplist(p.ships);
        for (auto ship : shiplist) {
          if (ship.protect.planet && ship.number != fromship && ship.alive &&
              ship.active) {
            if (laser_on(ship)) check_overload(ship, 0, &strength);

            strength = check_retal_strength(ship);

            auto const &s2sresult =
                shoot_ship_to_ship(ship, *from, strength, 0);
            if (s2sresult) {
              auto [damage, short_buf, long_buf] = *s2sresult;

              if (laser_on(ship))
                use_fuel(ship, 2.0 * (double)strength);
              else
                use_destruct(ship, strength);
              if (!from->alive) post(short_buf, NewsType::COMBAT);
              notify_star(Playernum, Governor, from->storbits, short_buf);
              warn(ship.owner, ship.governor, long_buf);
              notify(Playernum, Governor, long_buf);
            }
          }
          if (!from->alive) break;
        }
      }

      /* write the stuff to disk */
      putship(*from);
      putplanet(p, stars[from->storbits], (int)from->pnumorbits);
      deductAPs(g, APcount, from->storbits);

      free(from);
    } else
      free(from);
}
}  // namespace GB::commands
