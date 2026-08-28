// SPDX-License-Identifier: Apache-2.0

/// \file recording_session_registry.cc
/// \brief Implementation of RecordingSessionRegistry mock for testing
/// notifications.

module;

#include <cassert>

module test;

import gblib;
import std;

SessionRegistry& get_test_session_registry() {
  return get_null_session_registry();
}

std::vector<SessionInfo>
RecordingSessionRegistry::get_connected_sessions() const {
  return sessions;
}

bool RecordingSessionRegistry::is_connected(player_t player,
                                            governor_t gov) const {
  return std::ranges::any_of(sessions, [&](const auto& s) {
    return s.player == player && s.governor == gov && s.connected;
  });
}

void RecordingSessionRegistry::notify_race(player_t race,
                                           const std::string& message) {
  notifications.push_back({
      .player = race,
      .governor = 0,
      .message = message,
      .is_broadcast = true,
  });
}

bool RecordingSessionRegistry::notify_player(player_t race, governor_t gov,
                                             const std::string& message) {
  notifications.push_back({
      .player = race,
      .governor = gov,
      .message = message,
      .is_broadcast = false,
  });
  return true;
}

bool RecordingSessionRegistry::update_in_progress() const {
  return update_in_progress_flag;
}

void RecordingSessionRegistry::set_update_in_progress(bool val) {
  update_in_progress_flag = val;
}

bool RecordingSessionRegistry::has_received(player_t player,
                                            std::string_view needle) const {
  return std::ranges::any_of(notifications, [&](const auto& n) {
    return n.player == player && n.message.contains(needle);
  });
}

bool RecordingSessionRegistry::has_broadcast(std::string_view needle) const {
  return std::ranges::any_of(notifications, [&](const auto& n) {
    return n.is_broadcast && n.message.contains(needle);
  });
}

std::vector<std::string>
RecordingSessionRegistry::messages_for(player_t player) const {
  std::vector<std::string> msgs;
  for (const auto& n : notifications) {
    if (n.player == player) {
      msgs.push_back(n.message);
    }
  }
  return msgs;
}

void RecordingSessionRegistry::clear_notifications() {
  notifications.clear();
}
