// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

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
      if (p.info(Playernum - 1).route[i - 1].set) {
        star = p.info(Playernum - 1).route[i - 1].dest_star;
        planet = p.info(Playernum - 1).route[i - 1].dest_planet;
        load = p.info(Playernum - 1).route[i - 1].load;
        unload = p.info(Playernum - 1).route[i - 1].unload;
        std::string load_flags;
        load_flags += Fuel(load) ? 'f' : ' ';
        load_flags += Destruct(load) ? 'd' : ' ';
        load_flags += Resources(load) ? 'r' : ' ';
        load_flags += Crystals(load) ? 'x' : ' ';

        std::string unload_flags;
        unload_flags += Fuel(unload) ? 'f' : ' ';
        unload_flags += Destruct(unload) ? 'd' : ' ';
        unload_flags += Resources(unload) ? 'r' : ' ';
        unload_flags += Crystals(unload) ? 'x' : ' ';

        g.out << std::format(
            "{:2}  land {:2},{:2}   load: {}  unload: {}  -> {}/{}\n", i,
            p.info(Playernum - 1).route[i - 1].x,
            p.info(Playernum - 1).route[i - 1].y, load_flags, unload_flags,
            stars[star].get_name(), stars[star].get_planet_name(planet));
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
    if (p.info(Playernum - 1).route[i - 1].set) {
      star = p.info(Playernum - 1).route[i - 1].dest_star;
      planet = p.info(Playernum - 1).route[i - 1].dest_planet;
      load = p.info(Playernum - 1).route[i - 1].load;
      unload = p.info(Playernum - 1).route[i - 1].unload;
      std::string load_flags;
      if (load) {
        if (Fuel(load)) load_flags += 'f';
        if (Destruct(load)) load_flags += 'd';
        if (Resources(load)) load_flags += 'r';
        if (Crystals(load)) load_flags += 'x';
      }
      std::string unload_flags;
      if (unload) {
        if (Fuel(unload)) unload_flags += 'f';
        if (Destruct(unload)) unload_flags += 'd';
        if (Resources(unload)) unload_flags += 'r';
        if (Crystals(unload)) unload_flags += 'x';
      }
      g.out << std::format(
          "{:2}  land {:2},{:2}   {}{}  -> {}/{}\n", i,
          p.info(Playernum - 1).route[i - 1].x,
          p.info(Playernum - 1).route[i - 1].y,
          (load ? std::format("load: {}", load_flags) : std::string{}),
          (unload ? std::format("  unload: {}", unload_flags) : std::string{}),
          stars[star].get_name(), stars[star].get_planet_name(planet));
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
      p.info(Playernum - 1).route[i - 1].set = 1;
    else if (argv[2] == "deactivate")
      p.info(Playernum - 1).route[i - 1].set = 0;
    else {
      Place where{g, argv[2], true};
      if (!where.err) {
        if (where.level != ScopeLevel::LEVEL_PLAN) {
          g.out << "You have to designate a planet.\n";
          return;
        }
        p.info(Playernum - 1).route[i - 1].dest_star = where.snum;
        p.info(Playernum - 1).route[i - 1].dest_planet = where.pnum;
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
      if (x < 0 || x > p.Maxx() - 1 || y < 0 || y > p.Maxy() - 1) {
        g.out << "Bad sector coordinates.\n";
        return;
      }
      p.info(Playernum - 1).route[i - 1].x = x;
      p.info(Playernum - 1).route[i - 1].y = y;
    } else if (argv[2] == "load") {
      p.info(Playernum - 1).route[i - 1].load = 0;
      c = argv[3].c_str();
      while (*c) {
        if (*c == 'f') p.info(Playernum - 1).route[i - 1].load |= M_FUEL;
        if (*c == 'd') p.info(Playernum - 1).route[i - 1].load |= M_DESTRUCT;
        if (*c == 'r') p.info(Playernum - 1).route[i - 1].load |= M_RESOURCES;
        if (*c == 'x') p.info(Playernum - 1).route[i - 1].load |= M_CRYSTALS;
        c++;
      }
    } else if (argv[2] == "unload") {
      p.info(Playernum - 1).route[i - 1].unload = 0;
      c = argv[3].c_str();
      while (*c) {
        if (*c == 'f') p.info(Playernum - 1).route[i - 1].unload |= M_FUEL;
        if (*c == 'd') p.info(Playernum - 1).route[i - 1].unload |= M_DESTRUCT;
        if (*c == 'r') p.info(Playernum - 1).route[i - 1].unload |= M_RESOURCES;
        if (*c == 'x') p.info(Playernum - 1).route[i - 1].unload |= M_CRYSTALS;
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
