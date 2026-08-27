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

void telegram_star(EntityManager& em, starnum_t star, player_t sender,
                   governor_t sender_gov, const std::string& message) {
  const auto* star_ptr = em.peek_star(star);
  if (!star_ptr) return;

  for (player_t p = 1; p <= em.num_races(); p++) {
    if ((p != sender || sender_gov != 0) && isset(star_ptr->inhabited(), p)) {
      const auto* race = em.peek_race(p);
      if (race) {
        for (auto [i, gov] : race->active_governors()) {
          if (!(p == sender && i == sender_gov)) {
            push_telegram(em, p, i, message);
          }
        }
      }
    }
  }
}

void adjust_morale(Race& winner, Race& loser, int amount) {
  winner.morale += amount;
  loser.morale -= amount;
  winner.points[loser.Playernum.value - 1] += amount;
}

void add_to_queue(std::deque<std::string>& q, const std::string& b) {
  if (b.empty()) return;

  q.emplace_back(b);
}
