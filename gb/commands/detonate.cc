// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void detonate(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (argv.size() < 3) {
    std::string msg = "Syntax: '" + argv[0] + " <mine>'\n";
    notify(Playernum, Governor, msg);
    return;
  }

  Ship *s;
  shipnum_t shipno;
  shipnum_t nextshipno;

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        authorized(Governor, *s)) {
      if (s->type != ShipType::STYPE_MINE) {
        g.out << "That is not a mine.\n";
        free(s);
        continue;
      }
      if (!s->on) {
        g.out << "The mine is not activated.\n";
        free(s);
        continue;
      }
      if (s->docked || s->whatorbits == ScopeLevel::LEVEL_SHIP) {
        g.out << "The mine is docked or landed.\n";
        free(s);
        continue;
      }
      domine(*s, 1);
      free(s);
    } else
      free(s);
}
}  // namespace GB::commands
