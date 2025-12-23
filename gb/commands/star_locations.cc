// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void star_locations(const command_t& argv, GameObj& g) {
  int max = (argv.size() > 1) ? std::stoi(argv[1]) : 999999;

  for (auto star_handle : StarList(g.entity_manager)) {
    const auto& star = *star_handle;
    auto dist = std::sqrt(
        Distsq(star.xpos(), star.ypos(), g.lastx[1], g.lasty[1]));
    if (std::floor(dist) <= max) {
      g.out << std::format("({:2d}) {:20.20s} ({:8.0f},{:8.0f}) {:7.0f}\n",
                           star.star_id(), star.get_name(), star.xpos(),
                           star.ypos(), dist);
    }
  }
}
}  // namespace GB::commands
