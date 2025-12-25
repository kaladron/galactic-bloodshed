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
    g.out << std::format("Illegal name {}.\n", check ? "form" : "length");
    return;
  }

  if (argv[1] == "ship") {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      auto ship = g.entity_manager.get_ship(g.shipno);
      if (!ship.get()) {
        g.out << "Ship not found.\n";
        return;
      }
      ship->name() = namebuf;
      g.out << "Name set.\n";
      return;
    }
    g.out << "You have to 'cs' to a ship to name it.\n";
    return;
  }
  if (argv[1] == "class") {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      auto ship = g.entity_manager.get_ship(g.shipno);
      if (!ship.get()) {
        g.out << "Ship not found.\n";
        return;
      }
      if (ship->type() != ShipType::OTYPE_FACTORY) {
        g.out << "You are not at a factory!\n";
        return;
      }
      if (ship->on()) {
        g.out << "This factory is already on line.\n";
        return;
      }
      ship->shipclass() = namebuf;
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
    auto block_handle = g.entity_manager.get_block(Playernum);
    auto& block = *block_handle;
    block.name = namebuf;
    g.out << "Done.\n";
  } else if (argv[1] == "star") {
    if (g.level == ScopeLevel::LEVEL_STAR) {
      if (!g.race->God) {
        g.out << "Only dieties may name a star.\n";
        return;
      }
      auto star = g.entity_manager.get_star(g.snum);
      if (!star.get()) {
        g.out << "Star not found.\n";
        return;
      }
      star->set_name(namebuf);
    } else {
      g.out << "You have to 'cs' to a star to name it.\n";
      return;
    }
  } else if (argv[1] == "planet") {
    if (g.level == ScopeLevel::LEVEL_PLAN) {
      if (!g.race->God) {
        g.out << "Only deity can rename planets.\n";
        return;
      }
      auto star = g.entity_manager.get_star(g.snum);
      if (!star.get()) {
        g.out << "Star not found.\n";
        return;
      }
      star->set_planet_name(g.pnum, namebuf);
      deductAPs(g, APcount, g.snum);
    } else {
      g.out << "You have to 'cs' to a planet to name it.\n";
      return;
    }
  } else if (argv[1] == "race") {
    if (Governor) {
      g.out << "You are not authorized to do this.\n";
      return;
    }
    auto race = g.entity_manager.get_race(Playernum);
    if (!race.get()) {
      g.out << "Race not found.\n";
      return;
    }
    race->name = namebuf;
    g.out << std::format("Name changed to `{}'.\n", race->name);
  } else if (argv[1] == "governor") {
    auto race = g.entity_manager.get_race(Playernum);
    if (!race.get()) {
      g.out << "Race not found.\n";
      return;
    }
    race->governor[Governor].name = namebuf;
    g.out << std::format("Name changed to `{}'.\n",
                         race->governor[Governor].name);
  } else {
    g.out << "I don't know what you mean.\n";
    return;
  }
}
}  // namespace GB::commands