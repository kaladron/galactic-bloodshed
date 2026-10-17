// SPDX-License-Identifier: Apache-2.0

/// \file block.cc
/// \brief Display alliance block information.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;

module commands;

namespace GB::commands {

bool block(const command_t& argv, GameObj& g) {
  using GB::presentation::BlockDetailReport;
  using GB::presentation::BlockListReport;
  using GB::presentation::BlockMemberRow;
  using GB::presentation::BlockPlayerReport;
  using GB::presentation::BlockSummaryRow;
  using GB::presentation::BlockViewModel;

  player_t p;
  const auto* race = g.race;
  BlockViewModel vm{};

  if (argv.size() == 3 && argv[1] == "player") {
    p = get_player(g.entity_manager, argv[2]);
    if (p == 0) {
      g.out << "No such player.\n";
      return false;
    }
    const auto* r = g.entity_manager.peek_race(p);
    if (!r) {
      g.out << "Race not found.\n";
      return false;
    }

    BlockPlayerReport report{
        .player = p,
        .race_name = r->name,
    };
    for (const Block& block_i : BlockList::readonly(g.entity_manager)) {
      if (block_i.is_member(p)) {
        report.member_blocks.push_back(block_i.Playernum());
      }
      if (!block_i.is_pledged(p) && block_i.is_invited(p)) {
        report.invited_blocks.push_back(block_i.Playernum());
      }
      if (block_i.is_pledged(p) && !block_i.is_invited(p)) {
        report.pledged_blocks.push_back(block_i.Playernum());
      }
    }
    vm.player_report = std::move(report);
  } else if (argv.size() > 1) {
    p = get_player(g.entity_manager, argv[1]);
    if (p == 0) {
      g.out << "No such player,\n";
      return false;
    }
    const Block* block_p = nullptr;
    try {
      block_p = g.entity_manager.peek_block(p);
    } catch (const EntityNotFoundError&) {
      g.out << "Block not found.\n";
      return false;
    }

    BlockDetailReport report{
        .block_name = block_p->name(),
        .motto = block_p->motto(),
    };
    for (const Race& r : RaceList::readonly(g.entity_manager)) {
      if (!block_p->is_member(r.Playernum) || r.dissolved) continue;
      try {
        g.entity_manager.with_power(r.Playernum, [&](const Power& p_info) {
          report.members.push_back(BlockMemberRow{
              .player = r.Playernum,
              .name = r.name,
              .troops = race->estimate(p_info.troops(), r),
              .pop = race->estimate(p_info.popn(), r),
              .money = race->estimate(p_info.money(), r),
              .ships = race->estimate(p_info.ships_owned(), r),
              .planets = race->estimate(p_info.planets_owned(), r),
              .resource = race->estimate(p_info.resource(), r),
              .fuel = race->estimate(p_info.fuel(), r),
              .destruct = race->estimate(p_info.destruct(), r),
              .know = race->translation_for(r),
          });
        });
      } catch (const EntityNotFoundError&) {
        continue;
      }
    }
    vm.detail_report = std::move(report);
  } else {
    const auto* state = g.entity_manager.peek_server_state();
    const std::time_t update_time = state ? state->last_update_time : 0;

    BlockListReport report{
        .update_time = format_timestamp(update_time),
    };
    for (const Block& block_i : BlockList::readonly(g.entity_manager)) {
      player_t i = block_i.Playernum();
      if (block_i.members() == 0) continue;

      report.blocks.push_back(BlockSummaryRow{
          .player = i,
          .name = block_i.name(),
          .members = block_i.members(),
          .money = race->estimate(block_i.money(), i),
          .popn = race->estimate(block_i.popn(), i),
          .ships = race->estimate(block_i.ships_owned(), i),
          .systems = race->estimate(block_i.systems_owned(), i),
          .resource = race->estimate(block_i.resource(), i),
          .fuel = race->estimate(block_i.fuel(), i),
          .destruct = race->estimate(block_i.destruct(), i),
          .vps = race->estimate(block_i.VPs(), i),
          .know = race->translation_for(i),
      });
    }
    vm.list_report = std::move(report);
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor block_cmd{
    .name = "block",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "block [<player>|player <player>]",
    .description = "Display alliance block power reports and memberships",
    .handler = &block,
};

}  // namespace GB::commands
