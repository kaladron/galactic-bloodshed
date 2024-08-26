// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

namespace {
constexpr int MAX_OUTPUT = 32768;  // don't change this
}

void notify_race(const player_t race, const std::string &message) {
  if (update_flag) return;
  for (auto &d : descriptor_list) {
    if (d.connected && d.player == race) {
      queue_string(d, message);
    }
  }
}

bool notify(const player_t race, const governor_t gov,
            const std::string &message) {
  if (update_flag) return false;
  for (auto &d : descriptor_list)
    if (d.connected && d.player == race && d.governor == gov) {
      strstr_to_queue(d);  // Ensuring anything queued up is flushed out.
      queue_string(d, message);
      return true;
    }
  return false;
}

void d_think(const player_t Playernum, const governor_t Governor,
             const std::string &message) {
  for (auto &d : descriptor_list) {
    if (d.connected && d.player == Playernum && d.governor != Governor &&
        !races[d.player - 1].governor[d.governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void d_broadcast(const player_t Playernum, const governor_t Governor,
                 const std::string &message) {
  for (auto &d : descriptor_list) {
    if (d.connected && !(d.player == Playernum && d.governor == Governor) &&
        !races[d.player - 1].governor[d.governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void d_shout(const player_t Playernum, const governor_t Governor,
             const std::string &message) {
  for (auto &d : descriptor_list) {
    if (d.connected && !(d.player == Playernum && d.governor == Governor)) {
      queue_string(d, message);
    }
  }
}

void d_announce(const player_t Playernum, const governor_t Governor,
                const starnum_t star, const std::string &message) {
  for (auto &d : descriptor_list) {
    if (d.connected && !(d.player == Playernum && d.governor == Governor) &&
        (isset(stars[star].inhabited, d.player) || races[d.player - 1].God) &&
        d.snum == star &&
        !races[d.player - 1].governor[d.governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void warn_race(const player_t who, const std::string &message) {
  for (int i = 0; i <= MAXGOVERNORS; i++)
    if (races[who - 1].governor[i].active) warn(who, i, message);
}

void warn(const player_t who, const governor_t governor,
          const std::string &message) {
  if (!notify(who, governor, message) && !notify(who, 0, message))
    push_telegram(who, governor, message);
}

void warn_star(const player_t a, const starnum_t star,
               const std::string &message) {
  for (auto &race : races) {
    if (race.Playernum != a && isset(stars[star].inhabited, race.Playernum))
      warn_race(race.Playernum, message);
  }
}

void notify_star(const player_t a, const governor_t g, const starnum_t star,
                 const std::string &message) {
  for (auto &d : descriptor_list)
    if (d.connected && (d.player != a || d.governor != g) &&
        isset(stars[star].inhabited, d.player)) {
      queue_string(d, message);
    }
}

void adjust_morale(Race &winner, Race &loser, int amount) {
  winner.morale += amount;
  loser.morale -= amount;
  winner.points[loser.Playernum] += amount;
}

void add_to_queue(std::deque<std::string> &q, const std::string &b) {
  if (b.empty()) return;

  q.emplace_back(b);
}

int flush_queue(std::deque<std::string> &q, int n) {
  int really_flushed = 0;

  const std::string flushed_message = "<Output Flushed>\n";
  n += flushed_message.size();

  while (n > 0 && !q.empty()) {
    auto &p = q.front();
    n -= p.size();
    really_flushed += p.size();
    q.pop_front();
  }
  q.emplace_back(flushed_message);
  really_flushed -= flushed_message.size();
  return really_flushed;
}

void queue_string(DescriptorData &d, const std::string &b) {
  if (b.empty()) return;
  int space = MAX_OUTPUT - d.output_size - b.size();
  if (space < 0) d.output_size -= flush_queue(d.output, -space);
  add_to_queue(d.output, b);
  d.output_size += b.size();
}

//* Push contents of the stream to the queues
void strstr_to_queue(DescriptorData &d) {
  if (d.out.str().empty()) return;
  queue_string(d, d.out.str());
  d.out.clear();
  d.out.str("");
}