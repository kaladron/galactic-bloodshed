// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* capital.c -- designate a capital */

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void capital(const command_t& argv, GameObj& g) {
  const ap_t kAPCost = 50;

  if (g.governor() != 0) {
    g.out << "Only the leader may designate the capital.\n";
    return;
  }

  shipnum_t shipno = 0;
  if (argv.size() != 2)
    shipno = g.race->Gov_ship;
  else {
    auto shiptmp = string_to_shipnum(argv[1]);
    if (!shiptmp) {
      g.out << "Specify a valid ship number.\n";
      return;
    }
    shipno = *shiptmp;
  }

  const auto* s = g.entity_manager.peek_ship(shipno);
  if (!s) {
    g.out << "Change the capital to be what ship?\n";
    return;
  }

  if (argv.size() == 2) {
    starnum_t snum = s->storbits();
    if (testship(*s, g)) {
      g.out << "You can't do that!\n";
      return;
    }
    if (!landed(*s)) {
      g.out << "Try landing this ship first!\n";
      return;
    }

    const auto* star = g.entity_manager.peek_star(snum);
    if (!star) {
      g.out << "Star not found.\n";
      return;
    }

    if (!enufAP(g.entity_manager, g.player(), g.governor(),
                star->AP(g.player()), kAPCost)) {
      return;
    }
    if (s->type() != ShipType::OTYPE_GOV) {
      g.out << std::format("That ship is not a {}.\n",
                           Shipnames[ShipType::OTYPE_GOV]);
      return;
    }
    deductAPs(g, kAPCost, snum);

    // Get race for modification (RAII auto-saves on scope exit)
    auto race_handle = g.entity_manager.get_race(g.player());
    auto& race_mut = *race_handle;
    race_mut.Gov_ship = shipno;
  }

  g.out << std::format("Efficiency of governmental center: {:.0f}%.\n",
                       ((double)s->popn() / (double)max_crew(*s)) *
                           (100 - (double)s->damage()));
}
}  // namespace GB::commands
