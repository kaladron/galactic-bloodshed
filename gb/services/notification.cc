// SPDX-License-Identifier: Apache-2.0

/// \file notification.cc
/// \brief Implementation of notification and message routing services.

module;

import std;

module gb.services;

import gb.entities;

// Complex notification functions implemented using SessionRegistry primitives.
// These iterate over races/governors and use notify_player() for delivery.
// This avoids needing direct Session access, breaking the circular dependency.

void d_broadcast(SessionRegistry& registry, EntityManager& em, player_t sender,
                 governor_t sender_gov, const std::string& message) {
  // Send to all connected players except sender, respecting gag settings
  for (const Race& race : RaceList::readonly(em)) {
    const player_t p = race.Playernum;
    for (auto [g, gov] : race.active_governors()) {
      if (p == sender && g == sender_gov) continue;
      if (gov.toggle.gag) continue;

      registry.notify_player(p, g, message);
    }
  }
}

void d_announce(SessionRegistry& registry, EntityManager& em, player_t sender,
                governor_t sender_gov, starnum_t star,
                const std::string& message) {
  const auto* star_ptr = em.peek_star(star);
  if (!star_ptr) return;

  // Send to players who inhabit this star system, respecting gag
  for (const Race& race : RaceList::readonly(em)) {
    const player_t p = race.Playernum;
    // Must inhabit the star (or be God)
    if (!star_ptr->is_inhabited_by(p) && !race.God) continue;

    for (auto [g, gov] : race.active_governors()) {
      if (p == sender && g == sender_gov) continue;
      if (gov.toggle.gag) continue;

      registry.notify_player(p, g, message);
    }
  }
}

void d_think(SessionRegistry& registry, EntityManager& em, player_t race_num,
             governor_t sender_gov, const std::string& message) {
  const auto* race = em.peek_race(race_num);
  if (!race) return;

  // Send to other governors of the same race, respecting gag
  for (auto [g, gov] : race->active_governors()) {
    if (g == sender_gov) continue;
    if (gov.toggle.gag) continue;

    registry.notify_player(race_num, g, message);
  }
}

void d_shout(SessionRegistry& registry, EntityManager& em, player_t sender,
             governor_t sender_gov, const std::string& message) {
  // Send to all connected players except sender (ignores gag)
  for (const Race& race : RaceList::readonly(em)) {
    const player_t p = race.Playernum;
    for (auto [g, gov] : race.active_governors()) {
      if (p == sender && g == sender_gov) continue;

      registry.notify_player(p, g, message);
    }
  }
}

void warn_player(SessionRegistry& registry, EntityManager& em, player_t who,
                 governor_t gov, const std::string& message) {
  // During updates, skip real-time delivery
  if (registry.update_in_progress()) {
    push_telegram(em, who, gov, message);
    return;
  }

  // Try real-time delivery to the specific governor
  if (registry.notify_player(who, gov, message)) return;

  // Fall back to governor 0 if different
  if (gov != 0 && registry.notify_player(who, 0, message)) return;

  // No one connected, use telegram
  push_telegram(em, who, gov, message);
}

void warn_race(SessionRegistry& registry, EntityManager& em, player_t who,
               const std::string& message) {
  const auto* race = em.peek_race(who);
  if (!race) return;

  for (auto [g, gov] : race->active_governors()) {
    warn_player(registry, em, who, g, message);
  }
}

void notify_star(SessionRegistry& registry, EntityManager& em, player_t sender,
                 governor_t sender_gov, starnum_t star,
                 const std::string& message) {
  const auto* star_ptr = em.peek_star(star);
  if (!star_ptr) return;

  const bool in_update = registry.update_in_progress();
  for (const Race& race : RaceList::readonly(em)) {
    const player_t p = race.Playernum;
    if (p == sender && sender_gov == 0) continue;
    if (!star_ptr->is_inhabited_by(p)) continue;

    for (auto [g, gov] : race.active_governors()) {
      if (p == sender && g == sender_gov) continue;

      if (in_update || !registry.notify_player(p, g, message)) {
        push_telegram(em, p, g, message);
      }
    }
  }
}

void warn_star(SessionRegistry& registry, EntityManager& em, player_t sender,
               starnum_t star, const std::string& message) {
  const auto* star_ptr = em.peek_star(star);
  if (!star_ptr) return;

  // Send to all players who inhabit the star system (except sender)
  for (const Race& race : RaceList::readonly(em)) {
    const player_t p = race.Playernum;
    if (p == sender) continue;
    if (!star_ptr->is_inhabited_by(p)) continue;

    warn_race(registry, em, p, message);
  }
}
