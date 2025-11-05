// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void page(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = g.god ? 0 : 1;
  player_t i;
  int who;
  int gov;
  int to_block;

  if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1), APcount))
    return;

  gov = 0;  // TODO(jeffbailey): Init to zero.
  to_block = 0;
  if (argv[1] == "block") {
    to_block = 1;
    g.out << "Paging alliance block.\n";
    who = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
    gov = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
  } else {
    if (!(who = get_player(argv[1]))) {
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

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't make pages at universal scope.\n";
      break;
    default:
      stars[g.snum] = getstar(g.snum);
      if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1),
                  APcount)) {
        return;
      }

      const auto* race = g.entity_manager.peek_race(Playernum);
      if (!race) {
        g.out << "Race data not found.\n";
        return;
      }

      auto msg = std::format("{} \"{}\" page(s) you from the {} star system.\n",
                             race->name, race->governor[Governor].name,
                             stars[g.snum].get_name());

      if (to_block) {
        uint64_t dummy =
            Blocks[Playernum - 1].invite & Blocks[Playernum - 1].pledge;
        for (i = 1; i <= Num_races; i++)
          if (isset(dummy, i) && i != Playernum) notify_race(i, msg);
      } else {
        if (argv.size() > 1)
          notify(who, gov, msg);
        else
          notify_race(who, msg);
      }

      g.out << "Request sent.\n";
      break;
  }
  deductAPs(g, APcount, g.snum);
}
}  // namespace GB::commands
