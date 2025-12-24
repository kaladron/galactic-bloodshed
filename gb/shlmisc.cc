// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file shlmisc.cc
/// \brief Miscellaneous stuff included in the shell.

module;

import std.compat;

module gblib;

bool authorized(const governor_t Governor, const Ship& ship) {
  return (!Governor || ship.governor() == Governor);
}

/**
 * \brief Check is the ship is in the given input string.
 *
 * See start_shiplist's comment for more details.
 */
bool in_list(const player_t playernum, const std::string_view list,
             const Ship& s, shipnum_t* nextshipno) {
  if (s.owner() != playernum || !s.alive()) return false;

  if (list.length() == 0) return false;

  if (list[0] == '#' || std::isdigit(list[0])) {
    *nextshipno = 0;
    return true;
  }

  // Match either the ship letter or * for wildcard.
  for (const auto& p : list)
    if (p == Shipltrs[s.type()] || p == '*') return true;
  return false;
}

void DontOwnErr(int Playernum, int Governor, shipnum_t shipno) {
  std::string error_msg = std::format("You don't own ship #{}.\n", shipno);
  notify(Playernum, Governor, error_msg);
}

bool enufAP(player_t Playernum, governor_t Governor, ap_t have, ap_t needed) {
  if (have < needed) {
    std::string ap_msg =
        std::format("You don't have {} action points there.\n", needed);
    notify(Playernum, Governor, ap_msg);
    return false;
  }
  return true;
}

/**
 * \brief Find the player/governor that matches passwords
 * \param racepass Password for the race
 * \param govpass Password for the governor
 * \return player and governor numbers, or 0 and 0 if not found
 */
std::tuple<player_t, governor_t> getracenum(EntityManager& entity_manager,
                                            const std::string& racepass,
                                            const std::string& govpass) {
  // Iterate through all races to find password match
  for (auto race_handle : RaceList(entity_manager)) {
    const auto& race = race_handle.read();
    if (racepass == race.password) {
      for (governor_t j = 0; j <= MAXGOVERNORS; j++) {
        if (!race.governor[j].password.empty() &&
            govpass == race.governor[j].password) {
          return {race.Playernum, j};
        }
      }
    }
  }
  return {0, 0};
}

/* returns player # from string containing that players name or #. */
player_t get_player(EntityManager& em, const std::string& name) {
  player_t rnum = 0;

  if (name.empty()) return 0;

  if (isdigit(name[0])) {
    if ((rnum = std::stoi(name)) < 1 || rnum > em.num_races()) return 0;
    return rnum;
  }
  for (auto race_handle : RaceList(em)) {
    const auto& race = race_handle.read();
    if (name == race.name) return race.Playernum;
  }
  return 0;
}

void allocateAPs(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  ap_t maxalloc;
  ap_t alloc;

  if (g.level == ScopeLevel::LEVEL_UNIV) {
    std::string scope_msg =
        "Change scope to the system you which to transfer global APs to.\n";
    notify(Playernum, Governor, scope_msg);
    return;
  }
  alloc = std::stoi(argv[1]);
  if (alloc <= 0) {
    notify(Playernum, Governor,
           "You must specify a positive amount of APs to allocate.\n");
    return;
  }

  auto univ_handle = g.entity_manager.get_universe();
  auto& univ = *univ_handle;
  const auto& star = *g.entity_manager.peek_star(g.snum);
  maxalloc =
      std::min(univ.AP[Playernum - 1], LIMIT_APs - star.AP(Playernum - 1));
  if (alloc > maxalloc) {
    std::string max_msg =
        std::format("Illegal value ({}) - maximum = {}\n", alloc, maxalloc);
    notify(Playernum, Governor, max_msg);
    return;
  }
  univ.AP[Playernum - 1] -= alloc;
  auto star_handle = g.entity_manager.get_star(g.snum);
  auto& star_write = *star_handle;
  star_write.AP(Playernum - 1) =
      std::min(LIMIT_APs, star.AP(Playernum - 1) + alloc);
  std::string allocated_msg = "Allocated\n";
  notify(Playernum, Governor, allocated_msg);
}

void deductAPs(const GameObj& g, ap_t APs, ScopeLevel level) {
  if (APs == 0) return;

  if (level == ScopeLevel::LEVEL_UNIV) {
    auto univ_handle = g.entity_manager.get_universe();
    auto& univ = *univ_handle;
    univ.AP[g.player - 1] = std::max(0u, univ.AP[g.player - 1] - APs);
    return;
  }
}

void deductAPs(const GameObj& g, ap_t APs, starnum_t snum) {
  if (APs == 0) return;

  // Get star for modification (RAII auto-saves on scope exit)
  auto star_handle = g.entity_manager.get_star(snum);
  if (!star_handle.get()) {
    return;
  }

  auto& star = *star_handle;
  if (star.AP(g.player - 1) >= APs)
    star.AP(g.player - 1) -= APs;
  else {
    star.AP(g.player - 1) = 0;
    std::string cheater_msg = "WHOA!  You cheater!  Oooohh!  OOOOH!\n  I'm "
                              "tellllllllliiiiiiinnnnnnnnnggggggggg!!!!!!!\n";
    notify(g.player, g.governor, cheater_msg);
  }
}

std::optional<std::tuple<int, int, int, int>> get4args(std::string_view s) {
  if (s.empty()) return std::nullopt;

  // Find the comma separating x and y parts
  auto comma_pos = s.find(',');
  if (comma_pos == std::string_view::npos) return std::nullopt;

  std::string_view x_part = s.substr(0, comma_pos);
  std::string_view y_part = s.substr(comma_pos + 1);

  int xl = 0;
  int xh = 0;
  int yl = 0;
  int yh = 0;

  // Parse x coordinates (either "x" or "xl:xh")
  auto x_colon = x_part.find(':');
  if (x_colon != std::string_view::npos) {
    // Range format: xl:xh
    auto xl_result =
        std::from_chars(x_part.data(), x_part.data() + x_colon, xl);
    if (xl_result.ec != std::errc{} ||
        xl_result.ptr != x_part.data() + x_colon) {
      return std::nullopt;
    }

    auto xh_part = x_part.substr(x_colon + 1);
    auto xh_result =
        std::from_chars(xh_part.data(), xh_part.data() + xh_part.size(), xh);
    if (xh_result.ec != std::errc{} ||
        xh_result.ptr != xh_part.data() + xh_part.size()) {
      return std::nullopt;
    }
  } else {
    // Single value format: x
    auto x_result =
        std::from_chars(x_part.data(), x_part.data() + x_part.size(), xl);
    if (x_result.ec != std::errc{} ||
        x_result.ptr != x_part.data() + x_part.size()) {
      return std::nullopt;
    }
    xh = xl;
  }

  // Parse y coordinates (either "y" or "yl:yh")
  auto y_colon = y_part.find(':');
  if (y_colon != std::string_view::npos) {
    // Range format: yl:yh
    auto yl_result =
        std::from_chars(y_part.data(), y_part.data() + y_colon, yl);
    if (yl_result.ec != std::errc{} || yl_result.ptr != y_part.data() + y_colon)
      return std::nullopt;

    auto yh_part = y_part.substr(y_colon + 1);
    auto yh_result =
        std::from_chars(yh_part.data(), yh_part.data() + yh_part.size(), yh);
    if (yh_result.ec != std::errc{} ||
        yh_result.ptr != yh_part.data() + yh_part.size())
      return std::nullopt;
  } else {
    // Single value format: y
    auto y_result =
        std::from_chars(y_part.data(), y_part.data() + y_part.size(), yl);
    if (y_result.ec != std::errc{} ||
        y_result.ptr != y_part.data() + y_part.size())
      return std::nullopt;
    yh = yl;
  }

  return std::make_tuple(xl, xh, yl, yh);
}
