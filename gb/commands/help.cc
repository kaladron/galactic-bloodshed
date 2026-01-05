// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void help(const command_t& argv, GameObj& g) {
  if (argv.size() == 1) {
    // Display general help from HELP_FILE
    if (auto f = std::ifstream(HELP_FILE)) {
      std::string line;
      while (std::getline(f, line)) {
        g.out << line << "\n";
      }
    } else {
      g.out << "Help file not found.\n";
    }
  } else {
    // Display topic-specific help
    std::string filename = std::format("{}/{}.md", HELPDIR, argv[1]);
    if (auto f = std::ifstream(filename)) {
      std::string line;
      while (std::getline(f, line)) {
        g.out << line << "\n";
      }
      g.out << "----\nFinished.\n";
    } else {
      g.out << "Help on that subject unavailable.\n";
    }
  }
}
}  // namespace GB::commands
