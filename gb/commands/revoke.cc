// SPDX-License-Identifier: Apache-2.0

/// \file revoke.cc
/// \brief Revoke race governors and transfer assets command.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace {
money_t do_revoke(Race& race, const governor_t src_gov,
                  const governor_t tgt_gov, EntityManager& entity_manager) {
  push_telegram(entity_manager, race.Playernum, Race::leader_id,
                GB::presentation::render_revoke_header_telegram(
                    race.Playernum, src_gov, tgt_gov));

  for (auto star_handle : StarList(entity_manager)) {
    auto& star = *star_handle;
    if (star.governor(race.Playernum) == src_gov) {
      star.set_governor(race.Playernum, tgt_gov);
      push_telegram(
          entity_manager, race.Playernum, Race::leader_id,
          GB::presentation::render_revoke_star_telegram(star.get_name()));
    }
  }

  for (auto ship_handle :
       ShipList(entity_manager, ShipList::IterationType::AllAlive)) {
    const auto& ship_ro = ship_handle.peek();
    if (ship_ro.owner() == race.Playernum && ship_ro.governor() == src_gov) {
      auto& ship = *ship_handle;
      ship.governor() = tgt_gov;
      push_telegram(entity_manager, race.Playernum, Race::leader_id,
                    GB::presentation::render_revoke_ship_telegram(
                        ship.type_letter(), ship.number()));
    }
  }

  for (auto commod_handle : CommodList(entity_manager)) {
    commod_handle->transfer_governor(race.Playernum, src_gov, tgt_gov);
  }

  const money_t transferred = race.revoke_governor(src_gov, tgt_gov);
  push_telegram(entity_manager, race.Playernum, Race::leader_id,
                GB::presentation::render_revoke_money_telegram(transferred));
  push_telegram(
      entity_manager, race.Playernum, Race::leader_id,
      GB::presentation::render_revoke_footer_telegram(race.Playernum, src_gov));
  return transferred;
}
}  // namespace

namespace GB::commands {

bool revoke(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  const auto parsed_gov = scn::scan<governor_t::value_type>(argv[1], "{}");
  if (!parsed_gov || parsed_gov->value() < 1) {
    g.out << "No such governor.\n";
    return false;
  }
  const governor_t gov{parsed_gov->value()};

  if (Race::is_leader(gov)) {
    g.out << "You can't revoke your leadership!\n";
    return false;
  }
  governor_t j = Race::leader_id;
  if (argv.size() >= 4) {
    const auto parsed_j = scn::scan<governor_t::value_type>(argv[3], "{}");
    if (!parsed_j || parsed_j->value() < 1) {
      g.out << "You can't give stuff to that governor!\n";
      return false;
    }
    j = governor_t{parsed_j->value()};
  }
  bool success = false;
  money_t transferred = 0;
  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    if (!race.has_governor(gov)) {
      g.out << "That governor is not active.\n";
      return;
    }
    if (race.governor(gov).password != argv[2]) {
      g.out << "Incorrect password.\n";
      return;
    }
    if (!race.has_governor(j) || j == gov) {
      g.out << "Bad target governor.\n";
      return;
    }
    transferred = do_revoke(race, gov, j, g.entity_manager);
    success = true;
  });
  if (success) {
    g.present(GB::presentation::GovernorRevokedResult{
        .revoked_governor = gov,
        .target_governor = j,
        .money_transferred = transferred,
    });
  }
  return success;
}

const CommandDescriptor revoke_cmd{
    .name = "revoke",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "revoke <gov> <password> [<target>]",
    .description = "Revoke a race governor and transfer assets",
    .handler = &revoke,
};

}  // namespace GB::commands
