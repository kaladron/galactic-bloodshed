// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void name(const command_t& argv, GameObj& g) {
  ap_t APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  char* ch;
  int spaces;
  unsigned char check = 0;
  char string[1024];
  char tmp[128];

  if (argv.size() < 3 || !isalnum(argv[2][0])) {
    g.out << "Illegal name format.\n";
    return;
  }

  std::string namebuf = argv[2];
  for (int i = 3; i < argv.size(); i++) {
    sprintf(tmp, " %s", argv[i].c_str());
    namebuf += tmp;
  }
  sprintf(string, "%s", namebuf.c_str());

  /* make sure there are no ^'s or '/' in name,
    also make sure the name has at least 1 character in it */
  ch = string;
  spaces = 0;
  while (*ch != '\0') {
    check |=
        ((!isalnum(*ch) && !(*ch == ' ') && !(*ch == '.')) || (*ch == '/'));
    ch++;
    if (*ch == ' ') spaces++;
  }

  if (spaces == namebuf.size()) {
    g.out << "Illegal name.\n";
    return;
  }

  if (namebuf.size() < 1 || check) {
    notify(Playernum, Governor,
           std::format("Illegal name {}.\n", check ? "form" : "length"));
    return;
  }

  if (argv[1] == "ship") {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      auto ship = getship(g.shipno);
      ship->name = namebuf;
      putship(*ship);
      g.out << "Name set.\n";
      return;
    }
    g.out << "You have to 'cs' to a ship to name it.\n";
    return;
  }
  if (argv[1] == "class") {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      auto ship = getship(g.shipno);
      if (ship->type != ShipType::OTYPE_FACTORY) {
        g.out << "You are not at a factory!\n";
        return;
      }
      if (ship->on) {
        g.out << "This factory is already on line.\n";
        return;
      }
      ship->shipclass = namebuf;
      putship(*ship);
      g.out << "Class set.\n";
      return;
    }
    g.out << "You have to 'cs' to a factory to name the ship class.\n";
    return;
  }
  if (argv[1] == "block") {
    /* name your alliance block */
    if (Governor) {
      g.out << "You are not authorized to do this.\n";
      return;
    }
    Blocks[Playernum - 1].name = namebuf;
    Putblock(Blocks);
    g.out << "Done.\n";
  } else if (argv[1] == "star") {
    if (g.level == ScopeLevel::LEVEL_STAR) {
      auto& race = races[Playernum - 1];
      if (!race.God) {
        g.out << "Only dieties may name a star.\n";
        return;
      }
      stars[g.snum].set_name(namebuf);
      putstar(stars[g.snum], g.snum);
    } else {
      g.out << "You have to 'cs' to a star to name it.\n";
      return;
    }
  } else if (argv[1] == "planet") {
    if (g.level == ScopeLevel::LEVEL_PLAN) {
      stars[g.snum] = getstar(g.snum);
      auto& race = races[Playernum - 1];
      if (!race.God) {
        g.out << "Only deity can rename planets.\n";
        return;
      }
      stars[g.snum].set_planet_name(g.pnum, namebuf);
      putstar(stars[g.snum], g.snum);
      deductAPs(g, APcount, g.snum);
    } else {
      g.out << "You have to 'cs' to a planet to name it.\n";
      return;
    }
  } else if (argv[1] == "race") {
    auto& race = races[Playernum - 1];
    if (Governor) {
      g.out << "You are not authorized to do this.\n";
      return;
    }
    race.name = namebuf;
    notify(Playernum, Governor,
           std::format("Name changed to `{}'.\n", race.name));
    putrace(race);
  } else if (argv[1] == "governor") {
    auto& race = races[Playernum - 1];
    race.governor[Governor].name = namebuf;
    notify(
        Playernum, Governor,
        std::format("Name changed to `{}'.\n", race.governor[Governor].name));
    putrace(race);
  } else {
    g.out << "I don't know what you mean.\n";
    return;
  }
}
}  // namespace GB::commands