// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import session;
import std;

module commands;

namespace GB::commands {

void emulate(const command_t& argv, Session& session) {
  if (!session.god()) {
    session.out() << "Only God mode users can emulate.\n";
    return;
  }

  if (argv.size() < 3) {
    session.out() << "Usage: emulate <player> <governor>\n";
    return;
  }

  try {
    player_t new_player = std::stoi(std::string(argv[1]));
    governor_t new_gov = std::stoi(std::string(argv[2]));

    const auto* race = session.entity_manager().peek_race(new_player);
    if (!race) {
      session.out() << std::format("Player {} does not exist.\n", new_player);
    } else if (new_gov < 0 || new_gov > MAXGOVERNORS) {
      session.out() << std::format("Invalid governor {}. Must be 0-{}.\n",
                                   new_gov, MAXGOVERNORS);
    } else if (!race->governor[new_gov.value].active) {
      session.out() << std::format("Governor {} is not active.\n", new_gov);
    } else {
      // Switch to new player/governor
      session.set_player(new_player);
      session.set_governor(new_gov);
      session.set_god(false);  // When emulating, act as normal player

      session.out() << std::format("Emulating {} \"{}\" [{},{}]\n", race->name,
                                   race->governor[new_gov.value].name,
                                   new_player, new_gov);
    }
  } catch (const std::exception& /* e */) {
    session.out() << "Invalid player or governor number.\n";
  }
}

}  // namespace GB::commands
