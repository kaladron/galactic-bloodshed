// SPDX-License-Identifier: Apache-2.0

module;

import std;

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

/*utilities for dealing with ship lists */
void insert_sh_univ(universe_struct* sdata, Ship* s) {
  s->nextship() = sdata->ships;
  sdata->ships = s->number();
  s->whatorbits() = ScopeLevel::LEVEL_UNIV;
}

void insert_sh_star(Star& star, Ship* s) {
  s->nextship() = star.ships();
  star.ships() = s->number();
  s->whatorbits() = ScopeLevel::LEVEL_STAR;
}

void insert_sh_plan(Planet& pl, Ship* s) {
  s->nextship() = pl.ships();
  pl.ships() = s->number();
  s->whatorbits() = ScopeLevel::LEVEL_PLAN;
}

void insert_sh_ship(Ship* s, Ship* s2) {
  s->nextship() = s2->ships();
  s2->ships() = s->number();
  s->whatorbits() = ScopeLevel::LEVEL_SHIP;
  s->whatdest() = ScopeLevel::LEVEL_SHIP;
  s->destshipno() = s2->number();
}

/**
 * \brief Remove a ship from the list of ships orbiting the star
 * \arg s Ship to remove
 */
void remove_sh_star(EntityManager& entity_manager, Ship& s) {
  auto star_handle = entity_manager.get_star(s.storbits());
  auto& star = *star_handle;
  shipnum_t sh = star.ships();

  // If the ship is the first of the chain, point the star to the
  // next, which is zero if there are no other ships.
  if (sh == s.number()) {
    star.ships() = s.nextship();
  } else {
    ShipList ships(entity_manager, sh);
    for (auto ship_handle : ships) {
      Ship& s2 = *ship_handle;
      if (s2.nextship() == s.number()) {
        s2.nextship() = s.nextship();
        break;
      }
    }
  }

  // put in limbo - wait for insert_sh
  s.whatorbits() = ScopeLevel::LEVEL_UNIV;
  s.nextship() = 0;
}

/**
 * \brief Remove a ship from the list of ships orbiting the planet
 * \arg s Ship to remove
 */
void remove_sh_plan(EntityManager& entity_manager, Ship& s) {
  auto host_handle = entity_manager.get_planet(s.storbits(), s.pnumorbits());
  auto& host = *host_handle;
  shipnum_t sh = host.ships();

  // If the ship is the first of the chain, point the star to the
  // next, which is zero if there are no other ships.
  if (sh == s.number()) {
    host.ships() = s.nextship();
  } else {
    ShipList ships(entity_manager, sh);
    for (auto ship_handle : ships) {
      Ship& s2 = *ship_handle;
      if (s2.nextship() == s.number()) {
        s2.nextship() = s.nextship();
        break;
      }
    }
  }

  // put in limbo - wait for insert_sh
  s.whatorbits() = ScopeLevel::LEVEL_UNIV;
  s.nextship() = 0;
}

/**
 * \brief Remove a ship from the list of ships in the ship
 * \arg s Ship to remove
 */
void remove_sh_ship(EntityManager& entity_manager, Ship& s, Ship& host) {
  shipnum_t sh = host.ships();

  // If the ship is the first of the chain, point the ship to the
  // next, which is zero if there are no other ships.
  if (sh == s.number()) {
    host.ships() = s.nextship();
  } else {
    ShipList ships(entity_manager, sh);
    for (auto ship_handle : ships) {
      Ship& s2 = *ship_handle;
      if (s2.nextship() == s.number()) {
        s2.nextship() = s.nextship();
        break;
      }
    }
  }

  // put in limbo - wait for insert_sh
  s.whatorbits() = ScopeLevel::LEVEL_UNIV;
  s.nextship() = 0;
}
