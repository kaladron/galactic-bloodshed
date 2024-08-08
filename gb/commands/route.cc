// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"
#include "gb/place.h"

module commands;

namespace GB::commands {
void route(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): This seems to segfault with no args.
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  int i;
  int x;
  int y;
  unsigned char star;
  unsigned char planet;
  unsigned char load;
  unsigned char unload;
  const char *c;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor,
           "You have to 'cs' to a planet to examine routes.\n");
    return;
  }
  auto p = getplanet(g.snum, g.pnum);
  if (argv.size() == 1) { /* display all shipping routes that are active */
    for (i = 1; i <= MAX_ROUTES; i++)
      if (p.info[Playernum - 1].route[i - 1].set) {
        star = p.info[Playernum - 1].route[i - 1].dest_star;
        planet = p.info[Playernum - 1].route[i - 1].dest_planet;
        load = p.info[Playernum - 1].route[i - 1].load;
        unload = p.info[Playernum - 1].route[i - 1].unload;
        sprintf(buf, "%2d  land %2d,%2d   ", i,
                p.info[Playernum - 1].route[i - 1].x,
                p.info[Playernum - 1].route[i - 1].y);
        strcat(buf, "load: ");
        if (Fuel(load))
          strcat(buf, "f");
        else
          strcat(buf, " ");
        if (Destruct(load))
          strcat(buf, "d");
        else
          strcat(buf, " ");
        if (Resources(load))
          strcat(buf, "r");
        else
          strcat(buf, " ");
        if (Crystals(load)) strcat(buf, "x");
        strcat(buf, " ");

        strcat(buf, "  unload: ");
        if (Fuel(unload))
          strcat(buf, "f");
        else
          strcat(buf, " ");
        if (Destruct(unload))
          strcat(buf, "d");
        else
          strcat(buf, " ");
        if (Resources(unload))
          strcat(buf, "r");
        else
          strcat(buf, " ");
        if (Crystals(unload))
          strcat(buf, "x");
        else
          strcat(buf, " ");
        sprintf(temp, "  -> %s/%s\n", stars[star].name,
                stars[star].pnames[planet]);
        strcat(buf, temp);
        notify(Playernum, Governor, buf);
      }
    g.out << "Done.\n";
    return;
  }
  if (argv.size() == 2) {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return;
    }
    if (p.info[Playernum - 1].route[i - 1].set) {
      star = p.info[Playernum - 1].route[i - 1].dest_star;
      planet = p.info[Playernum - 1].route[i - 1].dest_planet;
      load = p.info[Playernum - 1].route[i - 1].load;
      unload = p.info[Playernum - 1].route[i - 1].unload;
      sprintf(buf, "%2d  land %2d,%2d   ", i,
              p.info[Playernum - 1].route[i - 1].x,
              p.info[Playernum - 1].route[i - 1].y);
      if (load) {
        sprintf(temp, "load: ");
        strcat(buf, temp);
        if (Fuel(load)) strcat(buf, "f");
        if (Destruct(load)) strcat(buf, "d");
        if (Resources(load)) strcat(buf, "r");
        if (Crystals(load)) strcat(buf, "x");
      }
      if (unload) {
        sprintf(temp, "  unload: ");
        strcat(buf, temp);
        if (Fuel(unload)) strcat(buf, "f");
        if (Destruct(unload)) strcat(buf, "d");
        if (Resources(unload)) strcat(buf, "r");
        if (Crystals(unload)) strcat(buf, "x");
      }
      sprintf(temp, "  ->  %s/%s\n", stars[star].name,
              stars[star].pnames[planet]);
      strcat(buf, temp);
      notify(Playernum, Governor, buf);
    }
    g.out << "Done.\n";
    return;
  }
  if (argv.size() == 3) {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return;
    }
    if (argv[2] == "activate")
      p.info[Playernum - 1].route[i - 1].set = 1;
    else if (argv[2] == "deactivate")
      p.info[Playernum - 1].route[i - 1].set = 0;
    else {
      Place where{g, argv[2], true};
      if (!where.err) {
        if (where.level != ScopeLevel::LEVEL_PLAN) {
          g.out << "You have to designate a planet.\n";
          return;
        }
        p.info[Playernum - 1].route[i - 1].dest_star = where.snum;
        p.info[Playernum - 1].route[i - 1].dest_planet = where.pnum;
        g.out << "Set.\n";
      } else {
        g.out << "Illegal destination.\n";
        return;
      }
    }
  } else {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return;
    }
    if (argv[2] == "land") {
      sscanf(argv[3].c_str(), "%d,%d", &x, &y);
      if (x < 0 || x > p.Maxx - 1 || y < 0 || y > p.Maxy - 1) {
        g.out << "Bad sector coordinates.\n";
        return;
      }
      p.info[Playernum - 1].route[i - 1].x = x;
      p.info[Playernum - 1].route[i - 1].y = y;
    } else if (argv[2] == "load") {
      p.info[Playernum - 1].route[i - 1].load = 0;
      c = argv[3].c_str();
      while (*c) {
        if (*c == 'f') p.info[Playernum - 1].route[i - 1].load |= M_FUEL;
        if (*c == 'd') p.info[Playernum - 1].route[i - 1].load |= M_DESTRUCT;
        if (*c == 'r') p.info[Playernum - 1].route[i - 1].load |= M_RESOURCES;
        if (*c == 'x') p.info[Playernum - 1].route[i - 1].load |= M_CRYSTALS;
        c++;
      }
    } else if (argv[2] == "unload") {
      p.info[Playernum - 1].route[i - 1].unload = 0;
      c = argv[3].c_str();
      while (*c) {
        if (*c == 'f') p.info[Playernum - 1].route[i - 1].unload |= M_FUEL;
        if (*c == 'd') p.info[Playernum - 1].route[i - 1].unload |= M_DESTRUCT;
        if (*c == 'r') p.info[Playernum - 1].route[i - 1].unload |= M_RESOURCES;
        if (*c == 'x') p.info[Playernum - 1].route[i - 1].unload |= M_CRYSTALS;
        c++;
      }
    } else {
      g.out << "What are you trying to do?\n";
      return;
    }
    g.out << "Set.\n";
  }
  putplanet(p, stars[g.snum], g.pnum);
}
}  // namespace GB::commands
