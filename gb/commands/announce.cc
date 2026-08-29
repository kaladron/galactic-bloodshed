// SPDX-License-Identifier: Apache-2.0

/// \file announce.cc
/// \brief Announce, broadcast, shout, or think messages across systems.

module;

import gb.entities;
import gb.services;
import notification;
import session;
import std;

module commands;

namespace {
enum class Communicate : char {
  ANN = ':',
  BROADCAST = '>',
  SHOUT = '!',
  THINK = '=',
  UNKNOWN = ' ',
};

Communicate get_mode(std::string_view command) {
  if (command == "announce" || command == ":") return Communicate::ANN;
  if (command == "broadcast" || command == "\"" || command == "'")
    return Communicate::BROADCAST;
  if (command == "shout" || command == "!") return Communicate::SHOUT;
  if (command == "think" || command == ";") return Communicate::THINK;
  return Communicate::UNKNOWN;
}
}  // namespace

namespace GB::commands {

bool announce(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  Communicate mode = get_mode(argv[0]);
  if (mode == Communicate::UNKNOWN) {
    g.out << "Not sure how you got here.\n";
    return false;
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

  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      if (mode == Communicate::ANN) mode = Communicate::BROADCAST;
      break;
    default: {
      const auto& star = *g.entity_manager.peek_star(g.snum());
      if ((mode == Communicate::ANN) &&
          !(star.is_inhabited_by(Playernum) || g.god())) {
        g.out << "You do not inhabit this system or have diety privileges.\n";
        return false;
      }
    }
  }

  std::string msg =
      std::format("{} \"{}\" [{},{}] {} {}\n", g.race->name,
                  g.race->governor[Governor.value].name, Playernum, Governor,
                  static_cast<char>(mode), message);

  switch (mode) {
    case Communicate::ANN:
      d_announce(g.session_registry, g.entity_manager, Playernum, Governor,
                 g.snum(), msg);
      break;
    case Communicate::BROADCAST:
      d_broadcast(g.session_registry, g.entity_manager, Playernum, Governor,
                  msg);
      break;
    case Communicate::SHOUT:
      d_shout(g.session_registry, g.entity_manager, Playernum, Governor, msg);
      break;
    case Communicate::THINK:
      d_think(g.session_registry, g.entity_manager, Playernum, Governor, msg);
      break;
    case Communicate::UNKNOWN:
      break;
  }
  return true;
}

namespace {
constexpr std::string_view announce_aliases[] = {":"};
constexpr std::string_view broadcast_aliases[] = {"\"", "'"};
constexpr std::string_view shout_aliases[] = {"!"};
constexpr std::string_view think_aliases[] = {";"};
}  // namespace

const CommandDescriptor announce_cmd{
    .name = "announce",
    .aliases = announce_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "announce <message>",
    .description =
        "Announce a message to all players present in current system",
    .handler = &announce,
};

const CommandDescriptor broadcast_cmd{
    .name = "broadcast",
    .aliases = broadcast_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "broadcast <message>",
    .description = "Broadcast a message universally to all players in the game",
    .handler = &announce,
};

const CommandDescriptor shout_cmd{
    .name = "shout",
    .aliases = shout_aliases,
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "shout <message>",
    .description = "Deity broadcast to announce universal admin notifications",
    .handler = &announce,
};

const CommandDescriptor think_cmd{
    .name = "think",
    .aliases = think_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "think <message>",
    .description = "Send a thought message internally to your own race",
    .handler = &announce,
};

}  // namespace GB::commands
