// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* dissolve.c -- commit suicide, nuke all ships and sectors; */

module;

import session;
import gblib;
import notification;
import std.compat;

module commands;

namespace GB::commands {
void dissolve(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  if (!DISSOLVE) {
    g.out << "Dissolve has been disabled. Please notify diety.\n";
    return;
  }

  if (Governor != 0) {
    g.out << "Only the leader may dissolve the race. The "
             "leader has been notified of your "
             "attempt!!!\n";
    g.session_registry.notify_player(
        Playernum, 0,
        std::format("Governor #{} has attempted to dissolve this race.\n",
                    Governor));
    return;
  }

  if (argv.size() < 3) {
    g.out << "Self-Destruct sequence requires passwords.\n";
    g.out << "Please use 'dissolve <race password> <leader "
             "password>'<option> to initiate\n";
    g.out << "self-destruct sequence.\n";
    return;
  }
  g.out << "WARNING!! WARNING!! WARNING!!\n";
  g.out << "-------------------------------\n";
  g.out << "Entering self destruct sequence!\n";

  std::string racepass(argv[1]);
  std::string govpass(argv[2]);

  bool waste = false;
  if (argv.size() > 3) {
    if (argv[3][0] == 'w') waste = true;
  }

  auto [player, governor] = getracenum(g.entity_manager, racepass, govpass);

  if (player.value == 0) {
    g.out << "Password mismatch, self-destruct not initiated!\n";
    return;
  }

  auto n_ships = g.entity_manager.num_ships();
  for (auto i = 1; i <= n_ships; i++) {
    auto ship_handle = g.entity_manager.get_ship(i);
    if (!ship_handle.get() || ship_handle->owner() != Playernum) continue;
    g.entity_manager.kill_ship(Playernum, *ship_handle);
    g.out << std::format("Ship #{}, self-destruct enabled\n", i);
  }

  for (auto star_handle : StarList(g.entity_manager)) {
    const auto& star = *star_handle;
    if (!isset(star.explored(), Playernum)) continue;

    for (auto planet_handle :
         PlanetList(g.entity_manager, star.star_id(), star)) {
      auto& pl = *planet_handle;
      if (pl.info(Playernum).explored && pl.info(Playernum).numsectsowned) {
        pl.info(Playernum).fuel = 0;
        pl.info(Playernum).destruct = 0;
        pl.info(Playernum).resource = 0;
        pl.info(Playernum).popn = 0;
        pl.info(Playernum).troops = 0;
        pl.info(Playernum).tax = 0;
        pl.info(Playernum).newtax = 0;
        pl.info(Playernum).crystals = 0;
        pl.info(Playernum).numsectsowned = 0;
        pl.info(Playernum).explored = 0;
        pl.info(Playernum).autorep = 0;
      }

      auto smap_handle =
          g.entity_manager.get_sectormap(star.star_id(), pl.planet_order());
      auto& smap = *smap_handle;
      for (auto& s : smap) {
        if (s.get_owner() == Playernum) {
          s.set_owner(0);
          s.set_troops(0);
          s.clear_popn();
          if (waste) s.set_condition(SectorType::SEC_WASTED);
        }
      }
    }
  }

  auto race_handle = g.entity_manager.get_race(Playernum);
  auto& race = *race_handle;
  race.dissolved = true;

  post(g.entity_manager,
       std::format("{} [{}] has dissolved.\n", race.name, Playernum),
       NewsType::DECLARATION);
}
}  // namespace GB::commands
