// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void star_locations(const command_t &argv, GameObj &g) {
  int max = (argv.size() > 1) ? std::stoi(argv[1]) : 999999;

  for (auto i = 0; i < Sdata.numstars; i++) {
    auto dist = std::sqrt(
        Distsq(stars[i].xpos(), stars[i].ypos(), g.lastx[1], g.lasty[1]));
    if (std::floor(dist) <= max) {
      g.out << std::format("({:2d}) {:20.20s} ({:8.0f},{:8.0f}) {:7.0f}\n", i,
                           stars[i].get_name(), stars[i].xpos(),
                           stars[i].ypos(), dist);
    }
  }
}
}  // namespace GB::commands
