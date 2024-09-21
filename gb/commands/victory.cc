// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void victory(const command_t &argv, GameObj &g) {
  int count = (argv.size() > 1) ? std::stoi(argv[1]) : Num_races;
  if (count > Num_races) count = Num_races;

  auto viclist = create_victory_list();

  g.out << "----==== PLAYER RANKINGS ====----\n";
  g.out << std::format("{:<4} {:<15} {:>8}\n", "No.", "Name",
                       (g.god ? "Score" : ""));
  for (int i = 0; auto &vic : viclist) {
    i++;
    if (g.god) {
      g.out << std::format("{:2} {} [{:2}] {:<15} {:5} {:6.2} {:3} {} {}\n", i,
                           vic.Thing ? 'M' : ' ', vic.racenum, vic.name,
                           vic.rawscore, vic.tech, vic.IQ,
                           races[vic.racenum - 1].password,
                           races[vic.racenum - 1].governor[0].password);
    } else {
      g.out << std::format("{:2}   [{:2}] {:<15.15}\n", i, vic.racenum,
                           vic.name);
    }
  }
}
}  // namespace GB::commands
