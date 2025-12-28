// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

namespace {
constexpr int MAX_OUTPUT = 32768;  // don't change this
}

void notify_race(const player_t race, const std::string& message) {
  if (update_flag) return;
  for (auto& d : descriptor_list) {
    if (d.connected && d.player == race) {
      queue_string(d, message);
    }
  }
}

bool notify(const player_t race, const governor_t gov,
            const std::string& message) {
  if (update_flag) return false;
  for (auto& d : descriptor_list)
    if (d.connected && d.player == race && d.governor == gov) {
      strstr_to_queue(d);  // Ensuring anything queued up is flushed out.
      queue_string(d, message);
      return true;
    }
  return false;
}

void d_think(EntityManager& entity_manager, const player_t Playernum,
             const governor_t Governor, const std::string& message) {
  for (auto& d : descriptor_list) {
    if (d.connected && d.player == Playernum && d.governor != Governor) {
      const auto* race = entity_manager.peek_race(d.player);
      if (race && !race->governor[d.governor].toggle.gag) {
        queue_string(d, message);
      }
    }
  }
}

void d_broadcast(EntityManager& entity_manager, const player_t Playernum,
                 const governor_t Governor, const std::string& message) {
  for (auto& d : descriptor_list) {
    if (d.connected && !(d.player == Playernum && d.governor == Governor)) {
      const auto* race = entity_manager.peek_race(d.player);
      if (race && !race->governor[d.governor].toggle.gag) {
        queue_string(d, message);
      }
    }
  }
}

void d_shout(const player_t Playernum, const governor_t Governor,
             const std::string& message) {
  for (auto& d : descriptor_list) {
    if (d.connected && !(d.player == Playernum && d.governor == Governor)) {
      queue_string(d, message);
    }
  }
}

void d_announce(EntityManager& entity_manager, const player_t Playernum,
                const governor_t Governor, const starnum_t star,
                const std::string& message) {
  const auto* star_ptr = entity_manager.peek_star(star);
  if (!star_ptr) return;

  for (auto& d : descriptor_list) {
    if (d.connected && !(d.player == Playernum && d.governor == Governor) &&
        d.snum == star) {
      const auto* race = entity_manager.peek_race(d.player);
      if (race && (isset(star_ptr->inhabited(), d.player) || race->God) &&
          !race->governor[d.governor].toggle.gag) {
        queue_string(d, message);
      }
    }
  }
}

// New implementation using EntityManager
void warn_race(EntityManager& entity_manager, const player_t who,
               const std::string& message) {
  const auto* race = entity_manager.peek_race(who);
  if (!race) return;

  for (int i = 0; i <= MAXGOVERNORS; i++)
    if (race->governor[i].active) warn(who, i, message);
}

void warn(const player_t who, const governor_t governor,
          const std::string& message) {
  if (!notify(who, governor, message) && !notify(who, 0, message))
    push_telegram(who, governor, message);
}

// New implementation using EntityManager
void warn_star(EntityManager& entity_manager, const player_t a,
               const starnum_t star, const std::string& message) {
  const auto* star_ptr = entity_manager.peek_star(star);
  if (!star_ptr) return;

  // Iterate through all potential players in the inhabited bitmap
  for (player_t p = 1; p <= entity_manager.num_races(); p++) {
    if (p != a && isset(star_ptr->inhabited(), p)) {
      warn_race(entity_manager, p, message);
    }
  }
}

void notify_star(EntityManager& entity_manager, const player_t a,
                 const governor_t g, const starnum_t star,
                 const std::string& message) {
  const auto* star_ptr = entity_manager.peek_star(star);
  if (!star_ptr) return;

  for (auto& d : descriptor_list)
    if (d.connected && (d.player != a || d.governor != g) &&
        isset(star_ptr->inhabited(), d.player)) {
      queue_string(d, message);
    }
}

void adjust_morale(Race& winner, Race& loser, int amount) {
  winner.morale += amount;
  loser.morale -= amount;
  winner.points[loser.Playernum] += amount;
}

void add_to_queue(std::deque<std::string>& q, const std::string& b) {
  if (b.empty()) return;

  q.emplace_back(b);
}

int flush_queue(std::deque<std::string>& q, int n) {
  int really_flushed = 0;

  const std::string flushed_message = "<Output Flushed>\n";
  n += flushed_message.size();

  while (n > 0 && !q.empty()) {
    auto& p = q.front();
    n -= p.size();
    really_flushed += p.size();
    q.pop_front();
  }
  q.emplace_back(flushed_message);
  really_flushed -= flushed_message.size();
  return really_flushed;
}

void queue_string(DescriptorData& d, const std::string& b) {
  if (b.empty()) return;
  int space = MAX_OUTPUT - d.output_size - b.size();
  if (space < 0) d.output_size -= flush_queue(d.output, -space);
  add_to_queue(d.output, b);
  d.output_size += b.size();
}

//* Push contents of the stream to the queues
void strstr_to_queue(DescriptorData& d) {
  // Legacy DescriptorData uses GameObj's internal_stream_
  // Cast back to stringstream to access .str()
  auto& sstream = dynamic_cast<std::stringstream&>(d.out);
  if (sstream.str().empty()) return;
  queue_string(d, sstream.str());
  sstream.clear();
  sstream.str("");
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
