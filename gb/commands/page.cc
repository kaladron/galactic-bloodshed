// SPDX-License-Identifier: Apache-2.0

module;

import session; // For SessionRegistry full definition - import before gblib
import gblib;
import notification;
import std.compat;

module commands;

namespace GB::commands {
void page(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = g.god() ? 0 : 1;
  player_t i;
  int who;
  int gov;
  int to_block;

  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!enufAP(g.entity_manager, Playernum, Governor, star.AP(Playernum - 1),
              APcount))
    return;

  gov = 0;  // TODO(jeffbailey): Init to zero.
  to_block = 0;
  if (argv[1] == "block") {
    to_block = 1;
    g.out << "Paging alliance block.\n";
    who = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
    gov = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
  } else {
    if (!(who = get_player(g.entity_manager, argv[1]))) {
      g.out << "No such player.\n";
      return;
    }
    const auto* alien = g.entity_manager.peek_race(who);
    if (!alien) {
      g.out << "Race not found.\n";
      return;
    }
    APcount *= !alien->God;
    if (argv.size() > 1) gov = std::stoi(argv[2]);
  }

  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't make pages at universal scope.\n";
      break;
    default:
      const auto& star = *g.entity_manager.peek_star(g.snum());
      if (!enufAP(g.entity_manager, Playernum, Governor, star.AP(Playernum - 1),
                  APcount)) {
        return;
      }

      auto msg = std::format(
          "{} \"{}\" page(s) you from the {} star system.\n", g.race->name,
          g.race->governor[Governor.value].name, star.get_name());

      if (to_block) {
        const auto* block_player = g.entity_manager.peek_block(Playernum);
        if (!block_player) {
          g.out << "Block not found.\n";
          return;
        }
        uint64_t allied_members = block_player->invite & block_player->pledge;
        for (i = 1; i <= g.entity_manager.num_races(); i++) {
          if (isset(allied_members, i) && i != Playernum) {
            g.session_registry.notify_race(i, msg);
          }
        }
      } else {
        if (argv.size() > 1) {
          g.session_registry.notify_player(who, gov, msg);
        } else {
          g.session_registry.notify_race(who, msg);
        }
      }

      g.out << "Request sent.\n";
      break;
  }
  deductAPs(g, APcount, g.snum());
}
}  // namespace GB::commands
