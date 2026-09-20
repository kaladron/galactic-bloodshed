// SPDX-License-Identifier: Apache-2.0

module;

import std;
#undef stdout

module gblib;

// Note: Notification functions moved to gb/services/notification.{cppm,cc}
// - d_broadcast, d_announce, d_think, d_shout (free functions with game logic)
// - warn_player, warn_race (free functions with game logic)
// - notify_race, notify_player (methods on SessionRegistry interface)
// - notify_star, warn_star (free functions with game logic)

void add_to_queue(std::deque<std::string>& q, const std::string& b) {
  if (b.empty()) return;

  q.emplace_back(b);
}
