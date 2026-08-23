// SPDX-License-Identifier: Apache-2.0

/// \file allocate.cc
/// \brief Allocate action points command implementation.

module commands;

import gblib;
import std;

namespace GB::commands {
bool allocate(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  ap_t alloc = std::stoi(argv[1]);
  if (alloc <= 0) {
    g.out << "You must specify a positive amount of APs to allocate.\n";
    return false;
  }

  auto univ_handle = g.entity_manager.get_universe();
  auto& univ = *univ_handle;
  const auto* star = g.entity_manager.peek_star(g.snum());
  ap_t maxalloc =
      std::min(univ.AP[Playernum.value - 1], LIMIT_APs - star->AP(Playernum));
  if (alloc > maxalloc) {
    g.out << std::format("Illegal value ({}) - maximum = {}\n", alloc,
                         maxalloc);
    return false;
  }
  univ.AP[Playernum.value - 1] -= alloc;
  auto star_handle = g.entity_manager.get_star(g.snum());
  auto& star_write = *star_handle;
  star_write.AP(Playernum) = std::min(LIMIT_APs, star->AP(Playernum) + alloc);
  g.out << "Allocated\n";
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
