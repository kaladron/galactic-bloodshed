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
  ap_t new_global_ap{0};
  g.entity_manager.mutate_universe([&](Universe& u) {
    u.deduct_AP(Playernum, alloc);
    new_global_ap = u.get_AP(Playernum);
  });
  ap_t new_star_ap{0};
  g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
    star.AP(Playernum) = std::min(LIMIT_APs, star.AP(Playernum) + alloc);
    new_star_ap = star.AP(Playernum);
  });
  g.present(GB::presentation::AllocateResult{
      .allocated = alloc,
      .star_ap = new_star_ap,
      .global_ap = new_global_ap,
  });
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
