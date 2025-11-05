// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace {
enum class Communicate {
  ANN = ':',
  BROADCAST = '>',
  SHOUT = '!',
  THINK = '=',
  UNKNOWN = ' ',
};

Communicate get_mode(const std::string &mode) {
  if (mode == "announce") return Communicate::ANN;
  if (mode == "broadcast" || mode == "'") return Communicate::BROADCAST;
  if (mode == "shout") return Communicate::SHOUT;
  if (mode == "think") return Communicate::THINK;

  return Communicate::UNKNOWN;
}
}  // namespace

namespace GB::commands {
void announce(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  Communicate mode = get_mode(argv[0]);
  if (mode == Communicate::UNKNOWN) {
    g.out << "Not sure how you got here.\n";
    return;
  }

  const auto* race = g.entity_manager.peek_race(Playernum);
  if (!race) {
    g.out << "Race data not found.\n";
    return;
  }

  if (mode == Communicate::SHOUT && !race->God) {
    g.out << "You are not privileged to use this command.\n";
    return;
  }

  std::stringstream ss_message;

  std::ranges::copy(argv | std::views::drop(1),
                    std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  // TODO(jeffbailey):
  // When LLVM libc++ supports join_with, we can use this instead of the above
  //  std::string message;
  //  message.assign_range(argv | std::views::drop(1) | std::views::join_with('
  //  '));

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (mode == Communicate::ANN) mode = Communicate::BROADCAST;
      break;
    default:
      if ((mode == Communicate::ANN) &&
          !(!!isset(stars[g.snum].inhabited(), Playernum) || race->God)) {
        g.out << "You do not inhabit this system or have diety privileges.\n";
        return;
      }
  }

  std::string msg = std::format("{} \"{}\" [{},{}] {} {}\n", race->name,
                                race->governor[Governor].name, Playernum,
                                Governor, std::to_underlying(mode), message);

  switch (mode) {
    case Communicate::ANN:
      d_announce(Playernum, Governor, g.snum, msg);
      break;
    case Communicate::BROADCAST:
      d_broadcast(Playernum, Governor, msg);
      break;
    case Communicate::SHOUT:
      d_shout(Playernum, Governor, msg);
      break;
    case Communicate::THINK:
      d_think(Playernum, Governor, msg);
      break;
    case Communicate::UNKNOWN:  // Impossible
      break;
  }
}
}  // namespace GB::commands
