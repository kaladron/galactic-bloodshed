// SPDX-License-Identifier: Apache-2.0

/// \file help.cc
/// \brief Display game documentation and help.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool help(const command_t& argv, GameObj& g) {
  if (argv.size() == 1) {
    // Display general help from HELP_FILE
    if (auto f = std::ifstream(HELP_FILE)) {
      std::string line;
      while (std::getline(f, line)) {
        g.out << line << "\n";
      }
    } else {
      g.out << "Help file not found.\n";
      return false;
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
      return false;
    }
  }
  return true;
}

const CommandDescriptor help_cmd{
    .name = "help",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "help [<topic>]",
    .description = "Display general help or documentation for a specific topic",
    .handler = &help,
};

}  // namespace GB::commands
