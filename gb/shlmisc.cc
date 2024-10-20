// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file shlmisc.cc
/// \brief Miscellaneous stuff included in the shell.

module;

import std.compat;

#include "gb/buffers.h"

module gblib;

bool authorized(const governor_t Governor, const Ship &ship) {
  return (!Governor || ship.governor == Governor);
}

/**
 * \brief Get start of ship lists from either a ship number or ScopeLevel
 *
 * start_shiplist and in_list work together so that a user can enter one of:
 * * 1234 - a ship number
 * * #1234 - a ship number prefixed by an octothorpe.
 * * f - a letter representing the type of ship
 * * frd - A sequence of letters representing the type of ship.  Processing
 * stops after first match.
 * * \* - An Asterisk as a wildcard for first match.
 *
 * When a letter or asterisk is given, the shiplist is taken from the current
 * scope.
 *
 * \param g Game object for scope
 * \param p String that might contain ship number
 * \return Ship number at the start of the ship list.
 */
shipnum_t start_shiplist(GameObj &g, const std::string_view p) {
  // If a ship number is given, return that.
  auto s = string_to_shipnum(p);
  if (s) {
    return *s;
  }

  // Ship number not given
  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      getsdata(&Sdata);
      return Sdata.ships;
    case ScopeLevel::LEVEL_STAR:
      stars[g.snum] = getstar(g.snum);
      return stars[g.snum].ships();
    case ScopeLevel::LEVEL_PLAN: {
      const auto planet = getplanet(g.snum, g.pnum);
      return planet.ships;
    }
    case ScopeLevel::LEVEL_SHIP:
      auto ship = getship(g.shipno);
      return ship->ships;
  }
}

/* Step through linked list at current player scope */
shipnum_t do_shiplist(Ship **s, shipnum_t *nextshipno) {
  shipnum_t shipno;
  if (!(shipno = *nextshipno)) return 0;

  if (!getship(s, shipno)) /* allocate memory, free in loop */
    return 0;
  *nextshipno = (*s)->nextship;
  return shipno;
}

/**
 * \brief Check is the ship is in the given input string.
 *
 * See start_shiplist's comment for more details.
 */
bool in_list(const player_t playernum, const std::string_view list,
             const Ship &s, shipnum_t *nextshipno) {
  if (s.owner != playernum || !s.alive) return false;

  if (list.length() == 0) return false;

  if (list[0] == '#' || std::isdigit(list[0])) {
    *nextshipno = 0;
    return true;
  }

  // Match either the ship letter or * for wildcard.
  for (const auto &p : list)
    if (p == Shipltrs[s.type] || p == '*') return true;
  return false;
}

void DontOwnErr(int Playernum, int Governor, shipnum_t shipno) {
  sprintf(buf, "You don't own ship #%lu.\n", shipno);
  notify(Playernum, Governor, buf);
}

bool enufAP(player_t Playernum, governor_t Governor, ap_t have, ap_t needed) {
  if (have < needed) {
    sprintf(buf, "You don't have %d action points there.\n", needed);
    notify(Playernum, Governor, buf);
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
std::tuple<player_t, governor_t> getracenum(const std::string &racepass,
                                            const std::string &govpass) {
  for (auto race : races) {
    if (racepass == race.password) {
      for (governor_t j = 0; j <= MAXGOVERNORS; j++) {
        if (*race.governor[j].password &&
            govpass == race.governor[j].password) {
          return {race.Playernum, j};
        }
      }
    }
  }
  return {0, 0};
}

/* returns player # from string containing that players name or #. */
player_t get_player(const std::string &name) {
  player_t rnum = 0;

  if (name.empty()) return 0;

  if (isdigit(name[0])) {
    if ((rnum = std::stoi(name)) < 1 || rnum > Num_races) return 0;
    return rnum;
  }
  for (const auto &race : races) {
    if (name == race.name) return race.Playernum;
  }
  return 0;
}

void allocateAPs(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  ap_t maxalloc;
  ap_t alloc;

  if (g.level == ScopeLevel::LEVEL_UNIV) {
    sprintf(
        buf,
        "Change scope to the system you which to transfer global APs to.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  alloc = std::stoi(argv[1]);
  if (alloc <= 0) {
    notify(Playernum, Governor,
           "You must specify a positive amount of APs to allocate.\n");
    return;
  }

  getsdata(&Sdata);
  maxalloc = std::min(Sdata.AP[Playernum - 1],
                      LIMIT_APs - stars[g.snum].AP(Playernum - 1));
  if (alloc > maxalloc) {
    sprintf(buf, "Illegal value (%d) - maximum = %d\n", alloc, maxalloc);
    notify(Playernum, Governor, buf);
    return;
  }
  Sdata.AP[Playernum - 1] -= alloc;
  putsdata(&Sdata);
  stars[g.snum] = getstar(g.snum);
  stars[g.snum].AP(Playernum - 1) =
      std::min(LIMIT_APs, stars[g.snum].AP(Playernum - 1) + alloc);
  putstar(stars[g.snum], g.snum);
  sprintf(buf, "Allocated\n");
  notify(Playernum, Governor, buf);
}

void deductAPs(const GameObj &g, ap_t APs, ScopeLevel level) {
  if (APs == 0) return;

  if (level == ScopeLevel::LEVEL_UNIV) {
    getsdata(&Sdata);
    Sdata.AP[g.player - 1] = std::max(0u, Sdata.AP[g.player - 1] - APs);
    putsdata(&Sdata);
    return;
  }
}

void deductAPs(const GameObj &g, ap_t APs, starnum_t snum) {
  if (APs == 0) return;

  stars[snum] = getstar(snum);

  if (stars[snum].AP(g.player - 1) >= APs)
    stars[snum].AP(g.player - 1) -= APs;
  else {
    stars[snum].AP(g.player - 1) = 0;
    sprintf(buf,
            "WHOA!  You cheater!  Oooohh!  OOOOH!\n  I'm "
            "tellllllllliiiiiiinnnnnnnnnggggggggg!!!!!!!\n");
    notify(g.player, g.governor, buf);
  }

  putstar(stars[snum], snum);
}

void get4args(const char *s, int *xl, int *xh, int *yl, int *yh) {
  char s1[17];
  char s2[17];
  const char *p = s;

  sscanf(p, "%[^,]", s1);
  while ((*p != ':') && (*p != ',')) p++;
  if (*p == ':') {
    sscanf(s1, "%d:%d", xl, xh);
    while (*p != ',') p++;
  } else if (*p == ',') {
    sscanf(s1, "%d", xl);
    *xh = (*xl);
  }

  sscanf(p, "%s", s2);
  while ((*p != ':') && (*p != '\0')) p++;
  if (*p == ':') {
    sscanf(s2, ",%d:%d", yl, yh);
  } else {
    sscanf(s2, ",%d,", yl);
    *yh = (*yl);
  }
}
