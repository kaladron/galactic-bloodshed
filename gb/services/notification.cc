// SPDX-License-Identifier: Apache-2.0

module;

import std;

module notification;

import gblib; // EntityManager, SessionRegistry, types, push_telegram

// Complex notification functions implemented using SessionRegistry primitives.
// These iterate over races/governors and use notify_player() for delivery.
// This avoids needing direct Session access, breaking the circular dependency.

void d_broadcast(SessionRegistry& registry, EntityManager& em, player_t sender,
                 governor_t sender_gov, const std::string& message) {
  // Send to all connected players except sender, respecting gag settings
  for (player_t p = 1; p <= em.num_races(); p++) {
    const auto* race = em.peek_race(p);
    if (!race) continue;

    for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
      if (!race->governor[g].active) continue;
      if (p == sender && g == sender_gov) continue;
      if (race->governor[g].toggle.gag) continue;

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
  for (player_t p = 1; p <= em.num_races(); p++) {
    const auto* race = em.peek_race(p);
    if (!race) continue;

    // Must inhabit the star (or be God)
    if (!isset(star_ptr->inhabited(), p) && !race->God) continue;

    for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
      if (!race->governor[g].active) continue;
      if (p == sender && g == sender_gov) continue;
      if (race->governor[g].toggle.gag) continue;

      registry.notify_player(p, g, message);
    }
  }
}

void d_think(SessionRegistry& registry, EntityManager& em, player_t race_num,
             governor_t sender_gov, const std::string& message) {
  const auto* race = em.peek_race(race_num);
  if (!race) return;

  // Send to other governors of the same race, respecting gag
  for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
    if (!race->governor[g].active) continue;
    if (g == sender_gov) continue;
    if (race->governor[g].toggle.gag) continue;

    registry.notify_player(race_num, g, message);
  }
}

void d_shout(SessionRegistry& registry, EntityManager& em, player_t sender,
             governor_t sender_gov, const std::string& message) {
  // Send to all connected players except sender (ignores gag)
  for (player_t p = 1; p <= em.num_races(); p++) {
    const auto* race = em.peek_race(p);
    if (!race) continue;

    for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
      if (!race->governor[g].active) continue;
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

  for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
    if (race->governor[g].active) {
      warn_player(registry, em, who, g, message);
    }
  }
}

void notify_star(SessionRegistry& registry, EntityManager& em, player_t sender,
                 governor_t sender_gov, starnum_t star,
                 const std::string& message) {
  const auto* star_ptr = em.peek_star(star);
  if (!star_ptr) return;

  // During updates, use telegram for all
  if (registry.update_in_progress()) {
    for (player_t p = 1; p <= em.num_races(); p++) {
      if (p == sender && sender_gov == 0) continue;
      if (!isset(star_ptr->inhabited(), p)) continue;

      const auto* race = em.peek_race(p);
      if (!race) continue;

      for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
        if (!race->governor[g].active) continue;
        if (p == sender && g == sender_gov) continue;
        push_telegram(em, p, g, message);
      }
    }
    return;
  }

  // Try real-time, fall back to telegram
  for (player_t p = 1; p <= em.num_races(); p++) {
    if (p == sender && sender_gov == 0) continue;
    if (!isset(star_ptr->inhabited(), p)) continue;

    const auto* race = em.peek_race(p);
    if (!race) continue;

    for (governor_t g = 0; g <= MAXGOVERNORS; g++) {
      if (!race->governor[g].active) continue;
      if (p == sender && g == sender_gov) continue;

      if (!registry.notify_player(p, g, message)) {
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
  for (player_t p = 1; p <= em.num_races(); p++) {
    if (p == sender) continue;
    if (!isset(star_ptr->inhabited(), p)) continue;

    warn_race(registry, em, p, message);
  }
}
