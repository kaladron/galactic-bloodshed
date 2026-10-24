// SPDX-License-Identifier: Apache-2.0

/// \file announce.cc
/// \brief Announce, broadcast, shout, or think messages across systems.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace {

bool emit_communication(GB::presentation::AnnounceMode mode,
                        const command_t& argv, GameObj& g) {
  const auto playernum = g.player();
  const auto governor = g.governor();
  const GB::presentation::AnnounceResult result{
      .mode = mode,
      .sender =
          GB::presentation::format_sender_tag(*g.race, playernum, governor),
      .message = argv | std::views::drop(1) | std::views::join_with(' ') |
                 std::ranges::to<std::string>(),
  };
  const auto msg = GB::presentation::render_announce_dispatch_message(result);

  switch (mode) {
    case GB::presentation::AnnounceMode::Announce:
      d_announce(g.session_registry, g.entity_manager, playernum, governor,
                 g.snum(), msg);
      break;
    case GB::presentation::AnnounceMode::Broadcast:
      d_broadcast(g.session_registry, g.entity_manager, playernum, governor,
                  msg);
      break;
    case GB::presentation::AnnounceMode::Shout:
      d_shout(g.session_registry, g.entity_manager, playernum, governor, msg);
      break;
    case GB::presentation::AnnounceMode::Think:
      d_think(g.session_registry, g.entity_manager, playernum, governor, msg);
      break;
  }

  g.present(result);
  return true;
}

bool broadcast_message(const command_t& argv, GameObj& g) {
  return emit_communication(GB::presentation::AnnounceMode::Broadcast, argv, g);
}

bool shout_message(const command_t& argv, GameObj& g) {
  return emit_communication(GB::presentation::AnnounceMode::Shout, argv, g);
}

bool think_message(const command_t& argv, GameObj& g) {
  return emit_communication(GB::presentation::AnnounceMode::Think, argv, g);
}

constexpr std::string_view announce_aliases[] = {":"};
constexpr std::string_view broadcast_aliases[] = {"\"", "'"};
constexpr std::string_view shout_aliases[] = {"!"};
constexpr std::string_view think_aliases[] = {";"};

}  // namespace

namespace GB::commands {

bool announce(const command_t& argv, GameObj& g) {
  if (g.level() == ScopeLevel::LEVEL_UNIV) {
    return emit_communication(GB::presentation::AnnounceMode::Broadcast, argv,
                              g);
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!star.is_inhabited_by(g.player()) && !g.god()) {
    g.out << "You do not inhabit this system or have diety privileges.\n";
    return false;
  }
  return emit_communication(GB::presentation::AnnounceMode::Announce, argv, g);
}

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
    .handler = &broadcast_message,
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
    .handler = &shout_message,
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
    .handler = &think_message,
};

}  // namespace GB::commands
