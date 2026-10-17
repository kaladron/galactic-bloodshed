// SPDX-License-Identifier: Apache-2.0

/// \file governors.cc
/// \brief Governor management commands.

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

bool governors(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (argv[0] == "governors") {
    if (argv.size() >= 4 && argv[2] == "password") {
      const auto parsed_gov = scn::scan<governor_t::value_type>(argv[1], "{}");
      if (!parsed_gov || parsed_gov->value() < 1) {
        g.out << "No such governor.\n";
        return false;
      }
      const governor_t gov{parsed_gov->value()};
      if (!g.is_leader() && Governor != gov) {
        g.out << "You can only change your own password.\n";
        return false;
      }
      bool success = false;
      g.entity_manager.mutate_race(Playernum, [&](Race& race) {
        if (race.Guest) {
          g.out << "Guest races cannot change passwords.\n";
          return;
        }
        if (!race.has_governor(gov)) {
          g.out << "That governor is inactive.\n";
          return;
        }
        race.governor(gov).password = argv[3];
        success = true;
      });
      if (success) {
        g.present(
            GB::presentation::GovernorPasswordChangedResult{.governor = gov});
      }
      return success;
    }

    GB::presentation::GovernorsViewModel vm{.is_leader = g.is_leader()};
    g.entity_manager.with_race(Playernum, [&](const Race& race) {
      for (auto [i, g_entry] : race.active_governors()) {
        std::string login_time = std::ctime(&g_entry.login);
        if (!login_time.empty() && login_time.back() == '\n') {
          login_time.pop_back();
        }
        vm.rows.push_back(GB::presentation::GovernorRow{
            .governor = i,
            .name = std::string(g_entry.name),
            .money = g_entry.money,
            .last_login = std::move(login_time),
            .password = g.is_leader()
                            ? std::optional<std::string>{g_entry.password}
                            : std::nullopt,
        });
      }
    });
    g.present(vm);
    return true;
  }

  if (argv[0] == "appoint") {
    if (!g.is_leader()) {
      g.out << "Only the race leader may appoint governors.\n";
      return false;
    }
    if (argv.size() < 2) {
      g.out << "Syntax: appoint [<gov>] <password>\n";
      return false;
    }
    if (argv.size() == 2) {
      governor_t gov{0};
      g.entity_manager.mutate_race(Playernum, [&](Race& race) {
        gov = race.appoint_governor({.password = argv[1]});
      });
      g.present(GB::presentation::GovernorAppointedResult{
          .governor = gov,
          .auto_assigned = true,
      });
      return true;
    }
    const auto parsed_gov = scn::scan<governor_t::value_type>(argv[1], "{}");
    if (!parsed_gov || parsed_gov->value() < 1) {
      g.out << "No such governor.\n";
      return false;
    }
    const governor_t gov{parsed_gov->value()};

    bool success = false;
    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      if (Race::is_leader(gov) || race.has_governor(gov)) {
        g.out << "That governor is already appointed.\n";
        return;
      }
      race.appoint_governor(gov, {.password = argv[2]});
      success = true;
    });
    if (success) {
      g.present(GB::presentation::GovernorAppointedResult{
          .governor = gov,
          .auto_assigned = false,
      });
    }
    return success;
  }

  if (argv[0] == "revoke") {
    if (!g.is_leader()) {
      g.out << "Only the race leader may revoke governors.\n";
      return false;
    }
    if (argv.size() < 3) {
      g.out << "Syntax: revoke <gov> <password> [<target>]\n";
      return false;
    }
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

  g.out << "Bad option.\n";
  return false;
}

namespace {
constexpr std::array<std::string_view, 2> kGovernorsAliases{"appoint",
                                                            "revoke"};
}

const CommandDescriptor governors_cmd{
    .name = "governors",
    .aliases = kGovernorsAliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "governors | appoint <gov> <password> | revoke <gov> <password> "
              "[<target>]",
    .description = "List, appoint, or revoke race governors",
    .handler = &governors,
};

}  // namespace GB::commands
