// SPDX-License-Identifier: Apache-2.0

/// \file allocate.cc
/// \brief Allocate action points command implementation.

module commands;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

namespace GB::commands {
bool allocate(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  auto scanned = scn::scan<int>(argv[1], "{}");
  if (!scanned || scanned->value() <= 0) {
    g.out << "You must specify a positive amount of APs to allocate.\n";
    return false;
  }
  const auto alloc = static_cast<ap_t>(scanned->value());

  const auto& univ = *g.entity_manager.peek_universe();
  const auto& star_ref = *g.entity_manager.peek_star(g.snum());
  ap_t maxalloc =
      std::min(univ.get_AP(Playernum), LIMIT_APs - star_ref.AP(Playernum));

  if (alloc > maxalloc) {
    g.out << std::format("Illegal value ({}) - maximum = {}\n", alloc,
                         maxalloc);
    return false;
  }
  g.entity_manager.mutate_universe(
      [&](Universe& u) { u.deduct_AP(Playernum, alloc); });
  g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
    star.AP(Playernum) = std::min(LIMIT_APs, star.AP(Playernum) + alloc);
  });
  g.present(GB::presentation::make_output_message("Allocated\n"));
  return true;
}

const CommandDescriptor allocate_cmd{
    .name = "allocate",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::non_universe(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "allocate <action points>",
    .description = "Transfer global action points to a star system",
    .handler = &allocate,
};

}  // namespace GB::commands
