// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void whois(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): ap_t APcount = 0;

  if (argv.size() <= 1) {
    whois({"whois", std::to_string(g.player)}, g);
    return;
  }

  for (const auto &player :
       std::ranges::subrange(argv.begin() + 1, argv.end())) {
    const auto j = std::stoi(player);

    if (j < 1 || j > Num_races) {
      g.out << std::format("Identify: Invalid player number #{}. Try again.\n",
                           j);
      continue;
    }

    const auto &race = races[j - 1];
    if (j == g.player) {
      g.out << std::format("[{:2d}, {}] {} \"{}\"\n", j, g.governor, race.name,
                           race.governor[g.governor].name);
    } else {
      g.out << std::format("[{:2d}] {}\n", j, race.name);
    }
  }
}
}  // namespace GB::commands