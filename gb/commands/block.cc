// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void block(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  player_t p;
  int dummy_;

  const auto* race = g.race;  // Use pre-populated race from GameObj

  if (argv.size() == 3 && argv[1] == "player") {
    if (!(p = get_player(g.entity_manager, argv[2]))) {
      g.out << "No such player.\n";
      return;
    }
    const auto* r = g.entity_manager.peek_race(p);
    if (!r) {
      g.out << "Race not found.\n";
      return;
    }
    dummy_ = 0; /* Used as flag for finding a block */
    notify(Playernum, Governor,
           std::format("Race #{} [{}] is a member of ", p, r->name));
    for (int i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i) continue;
      if (isset(block_i->pledge, p) && isset(block_i->invite, p)) {
        notify(Playernum, Governor,
               std::format("{}{}", (dummy_ == 0) ? " " : ", ", i));
        dummy_ = 1;
      }
    }
    if (dummy_ == 0)
      g.out << "no blocks\n";
    else
      g.out << "\n";

    dummy_ = 0; /* Used as flag for finding a block */
    notify(Playernum, Governor,
           std::format("Race #{} [{}] has been invited to join ", p, r->name));
    for (int i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i) continue;
      if (!isset(block_i->pledge, p) && isset(block_i->invite, p)) {
        notify(Playernum, Governor,
               std::format("{}{}", (dummy_ == 0) ? " " : ", ", i));
        dummy_ = 1;
      }
    }
    if (dummy_ == 0)
      g.out << "no blocks\n";
    else
      g.out << "\n";

    dummy_ = 0; /* Used as flag for finding a block */
    notify(Playernum, Governor,
           std::format("Race #{} [{}] has pledged ", p, r->name));
    for (int i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i) continue;
      if (isset(block_i->pledge, p) && !isset(block_i->invite, p)) {
        notify(Playernum, Governor,
               std::format("{}{}", (dummy_ == 0) ? " " : ", ", i));
        dummy_ = 1;
      }
    }
    if (!dummy_)
      g.out << "no blocks\n";
    else
      g.out << "\n";
  } else if (argv.size() > 1) {
    if (!(p = get_player(g.entity_manager, argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
    /* list the players who are in this alliance block */
    const auto* block_p = g.entity_manager.peek_block(p);
    if (!block_p) {
      g.out << "Block not found.\n";
      return;
    }
    uint64_t allied_members = (block_p->invite & block_p->pledge);
    notify(Playernum, Governor,
           std::format("         ========== {} Power Report ==========\n",
                       block_p->name));
    notify(Playernum, Governor,
           std::format("                 {:<64.64}\n", block_p->motto));
    notify(Playernum, Governor,
           "  #  Name              troops  pop  money ship  plan  res fuel "
           "dest know\n");

    for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
      if (!isset(allied_members, i)) continue;
      const auto* r = g.entity_manager.peek_race(i);
      if (!r || r->dissolved) continue;
      const auto* power_ptr = g.entity_manager.peek_power(r->Playernum);
      if (!power_ptr) continue;
      g.out << std::format("{:2d} {:<20.20s} ", r->Playernum, r->name);
      g.out << std::format("{:5s}",
                           Estimate_i(power_ptr->troops, *race, r->Playernum));
      g.out << std::format("{:5s}",
                           Estimate_i(power_ptr->popn, *race, r->Playernum));
      g.out << std::format("{:5s}",
                           Estimate_i(power_ptr->money, *race, r->Playernum));
      g.out << std::format(
          "{:5s}", Estimate_i(power_ptr->ships_owned, *race, r->Playernum));
      g.out << std::format(
          "{:5s}", Estimate_i(power_ptr->planets_owned, *race, r->Playernum));
      g.out << std::format(
          "{:5s}", Estimate_i(power_ptr->resource, *race, r->Playernum));
      g.out << std::format("{:5s}",
                           Estimate_i(power_ptr->fuel, *race, r->Playernum));
      g.out << std::format(
          "{:5s}", Estimate_i(power_ptr->destruct, *race, r->Playernum));
      g.out << std::format(" {:3d}%%\n", race->translate[r->Playernum - 1]);
    }
  } else { /* list power report for all the alliance blocks (as of the last
              update) */
    notify(
        Playernum, Governor,
        std::format("         ========== Alliance Blocks as of {} ==========\n",
                    std::asctime(std::localtime(&Power_blocks.time))));
    notify(Playernum, Governor,
           " #  Name             memb money popn ship  sys  res fuel dest  VPs "
           "know\n");
    for (auto i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i || !block_i->VPs) continue;
      g.out << std::format("{:2d} {:<19.19}{:3d}", i, block_i->name,
                           Power_blocks.members[i - 1]);
      g.out << std::format(
          "{:5s}", Estimate_i((int)(Power_blocks.money[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}", Estimate_i((int)(Power_blocks.popn[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}",
          Estimate_i((int)(Power_blocks.ships_owned[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}",
          Estimate_i((int)(Power_blocks.systems_owned[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}", Estimate_i((int)(Power_blocks.resource[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}", Estimate_i((int)(Power_blocks.fuel[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}", Estimate_i((int)(Power_blocks.destruct[i - 1]), *race, i));
      g.out << std::format(
          "{:5s}", Estimate_i((int)(Power_blocks.VPs[i - 1]), *race, i));
      g.out << std::format(" {:3d}%%\n", race->translate[i - 1]);
    }
  }
}
}  // namespace GB::commands
