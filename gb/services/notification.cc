// SPDX-License-Identifier: Apache-2.0

module;

import std;

module notification;

import session; // SessionRegistry, Session
import gblib;   // EntityManager, types

// Note: notify_race() and notify_player() are now methods on SessionRegistry
// (defined in session.cc). This file contains only functions with game logic.

void d_broadcast(SessionRegistry& registry, EntityManager& em, player_t sender,
                 governor_t sender_gov, const std::string& message) {
  registry.for_each_session([&](Session& session) {
    if (session.connected() &&
        !(session.player() == sender && session.governor() == sender_gov)) {
      const auto* race = em.peek_race(session.player());
      if (race && !race->governor[session.governor()].toggle.gag) {
        session.out() << message;
      }
    }
  });
}

void d_announce(SessionRegistry& registry, EntityManager& em, player_t sender,
                governor_t sender_gov, starnum_t star,
                const std::string& message) {
  const auto* star_ptr = em.peek_star(star);
  if (!star_ptr) return;

  registry.for_each_session([&](Session& session) {
    if (session.connected() &&
        !(session.player() == sender && session.governor() == sender_gov) &&
        session.snum() == star) {
      const auto* race = em.peek_race(session.player());
      if (race &&
          (isset(star_ptr->inhabited(), session.player()) || race->God) &&
          !race->governor[session.governor()].toggle.gag) {
        session.out() << message;
      }
    }
  });
}

void d_think(SessionRegistry& registry, EntityManager& em, player_t race_num,
             governor_t sender_gov, const std::string& message) {
  registry.for_each_session([&](Session& session) {
    if (session.connected() && session.player() == race_num &&
        session.governor() != sender_gov) {
      const auto* race = em.peek_race(session.player());
      if (race && !race->governor[session.governor()].toggle.gag) {
        session.out() << message;
      }
    }
  });
}

void d_shout(SessionRegistry& registry, player_t sender, governor_t sender_gov,
             const std::string& message) {
  registry.for_each_session([&](Session& session) {
    if (session.connected() &&
        !(session.player() == sender && session.governor() == sender_gov)) {
      session.out() << message;
    }
  });
}

void warn_player(SessionRegistry& registry, player_t who, governor_t gov,
                 const std::string& message) {
  // During updates, skip real-time (matches original update_flag behavior)
  if (registry.update_in_progress()) {
    push_telegram(who, gov, message);
    return;
  }
  // Try real-time, fall back to telegram if not connected
  if (!registry.notify_player(who, gov, message) &&
      !registry.notify_player(who, 0, message)) {
    push_telegram(who, gov, message);
  }
}

void warn_race(SessionRegistry& registry, EntityManager& em, player_t who,
               const std::string& message) {
  const auto* race = em.peek_race(who);
  if (!race) return;

  for (int i = 0; i <= MAXGOVERNORS; i++) {
    if (race->governor[i].active) {
      warn_player(registry, who, i, message);
    }
  }
}
