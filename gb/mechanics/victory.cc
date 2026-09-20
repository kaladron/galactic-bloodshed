// SPDX-License-Identifier: Apache-2.0

/// \file victory.cc
/// \brief Victory condition calculation and victory rankings generation.

module;

import std;

module gb.mechanics;

std::vector<Victory> create_victory_list(EntityManager& entity_manager) {
  std::vector<Victory> victories;
  for (const Race& race : RaceList::readonly(entity_manager)) {
    Victory vic{.racenum = race.Playernum,
                .name = race.name,
                .tech = race.tech,
                .thing = race.Metamorph,
                .iq = race.IQ,
                .rawscore = race.victory_score};
    if (race.God || race.Guest || race.dissolved) vic.no_count = true;
    victories.emplace_back(vic);
  }
  std::ranges::sort(victories, std::less());

  return victories;
}

void compute_power_blocks(EntityManager& entity_manager) {
  /* compute alliance block power */
  const std::time_t now = std::time(nullptr);
  entity_manager.mutate_server_state(
      [now](ServerState& state) { state.last_update_time = now; });

  for (const Race& race_i : RaceList::readonly(entity_manager)) {
    const player_t i = race_i.Playernum;

    try {
      entity_manager.mutate_block(i.value, [&](block& block_i) {
        block_i.clear_power_stats();

        for (const Race& race_j : RaceList::readonly(entity_manager)) {
          const player_t j = race_j.Playernum;

          if (block_i.is_member(j)) {
            try {
              const auto* power_ptr =
                  entity_manager.peek_power(powernum_t{j.value});
              block_i.accumulate_member_power(*power_ptr);
            } catch (const EntityNotFoundError&) {
              continue;
            }
          }
        }
      });
    } catch (const EntityNotFoundError&) {
      continue;
    }
  }
}
