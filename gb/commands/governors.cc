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

namespace GB::commands {

bool governors(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

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

const CommandDescriptor governors_cmd{
    .name = "governors",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "governors [<gov> password <password>]",
    .description = "List race governors or change governor password",
    .handler = &governors,
};

}  // namespace GB::commands
