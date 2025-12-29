// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file grant.cc

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void grant(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  // ap_t APcount = 0; TODO(jeffbailey);
  governor_t gov;

  if (argv.size() < 3) {
    g.out << "Syntax: grant <governor> star\n";
    g.out << "        grant <governor> ship <shiplist>\n";
    g.out << "        grant <governor> money <amount>\n";
    return;
  }
  if ((gov = std::stoi(argv[1])) > MAXGOVERNORS) {
    g.out << "Bad governor number.\n";
    return;
  }

  if (!g.race->governor[gov].active) {
    g.out << "That governor is not active.\n";
    return;
  }

  auto race_handle = g.entity_manager.get_race(Playernum);
  auto& race = *race_handle;

  if (argv[2] == "star") {
    if (g.level() != ScopeLevel::LEVEL_STAR) {
      g.out << "Please cs to the star system first.\n";
      return;
    }
    int snum = g.snum();
    auto star_handle = g.entity_manager.get_star(snum);
    if (!star_handle.get()) {
      g.out << "Star not found.\n";
      return;
    }
    star_handle->governor(Playernum - 1) = gov;
    warn(Playernum, gov,
         std::format("\"{}\" has granted you control of the /{} star system.\n",
                     race.governor[Governor].name, star_handle->get_name()));
  } else if (argv[2] == "ship") {
    ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
    for (auto ship_handle : ships) {
      Ship& ship = *ship_handle;

      if (!ship_matches_filter(argv[3], ship)) continue;
      if (!authorized(Governor, ship)) continue;

      ship.governor() = gov;
      warn(Playernum, gov,
           std::format("\"{}\" granted you {} at {}\n",
                       race.governor[Governor].name, ship_to_string(ship),
                       prin_ship_orbits(g.entity_manager, ship)));
      g.out << std::format("{} granted to \"{}\"\n", ship_to_string(ship),
                           race.governor[gov].name);
    }
  } else if (argv[2] == "money") {
    long amount = 0;
    if (argv.size() < 4) {
      g.out << "Indicate the amount of money.\n";
      return;
    }
    amount = std::stoi(argv[3]);
    if (amount < 0 && Governor) {
      g.out << "Only leaders may make take away money.\n";
      return;
    }
    if (amount > race.governor[Governor].money)
      amount = race.governor[Governor].money;
    else if (-amount > race.governor[gov].money)
      amount = -race.governor[gov].money;
    if (amount >= 0)
      g.out << std::format("{} money granted to \"{}\".\n", amount,
                           race.governor[gov].name);
    else
      g.out << std::format("{} money deducted from \"{}\".\n", -amount,
                           race.governor[gov].name);
    if (amount >= 0)
      warn(Playernum, gov,
           std::format("\"{}\" granted you {} money.\n",
                       race.governor[Governor].name, amount));
    else
      warn(Playernum, gov,
           std::format("\"{}\" docked you {} money.\n",
                       race.governor[Governor].name, -amount));
    race.governor[Governor].money -= amount;
    race.governor[gov].money += amount;
    return;
  } else
    g.out << "You can't grant that.\n";
}
}  // namespace GB::commands
