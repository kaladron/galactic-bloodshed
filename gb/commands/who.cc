// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import session;
import tabulate;
import std;

module commands;

namespace GB::commands {

void who(const command_t& /* argv */, Session& session) {
  std::time_t now = std::time(nullptr);
  bool is_god = false;
  int coward_count = 0;

  session.out() << std::format("Current Players: {}", std::ctime(&now));

  const auto* current_race =
      session.entity_manager().peek_race(session.player());
  if (current_race) {
    is_god = current_race->God;
  }

  tabulate::Table table;
  table.add_row({"Race", "Governor", "Player", "Idle", "Star", "Flags"});

  // Iterate over all sessions
  session.registry().for_each_session([&](Session& s) {
    if (s.connected() && !s.god()) {
      const auto* r = s.entity_manager().peek_race(s.player());
      if (!r) return;

      // Check if this player should be visible
      bool is_visible = !r->governor[s.governor()].toggle.invisible ||
                        s.player() == session.player() || is_god;

      if (is_visible) {
        std::string gov_name =
            std::format("\"{}\"", r->governor[s.governor()].name);
        const auto* star = s.entity_manager().peek_star(s.snum());
        std::string star_name = is_god && star ? star->get_name() : "";
        std::time_t idle_seconds = now - s.last_time();
        std::string player_gov =
            std::format("[{},{}]", s.player(), s.governor());
        std::string idle_str = std::format("{}s", idle_seconds);

        std::vector<std::string> flags;
        if (r->governor[s.governor()].toggle.gag) flags.push_back("GAG");
        if (r->governor[s.governor()].toggle.invisible)
          flags.push_back("INVISIBLE");
        std::string flags_str;
        for (std::size_t i = 0; i < flags.size(); ++i) {
          if (i > 0) flags_str += " ";
          flags_str += flags[i];
        }

        table.add_row(
            {r->name, gov_name, player_gov, idle_str, star_name, flags_str});
      } else if (!is_god) {
        coward_count++;  // Non-God player sees someone invisible
      }
    }
  });

  session.out() << table << "\n";

  if (SHOW_COWARDS) {
    session.out() << std::format("And {} coward{}.\n", coward_count,
                                 (coward_count == 1) ? "" : "s");
  } else {
    session.out() << "Finished.\n";
  }
}

}  // namespace GB::commands
