// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file shlmisc.cc
/// \brief Miscellaneous stuff included in the shell.

#include "gb/shlmisc.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

/**
 * \brief Convert input string to a shipnum_t
 * \param s User-provided input string
 * \return If the user provided a valid number, return it.
 */
std::optional<shipnum_t> string_to_shipnum(std::string_view s) {
  if (s.size() > 1 && s[0] == '#') {
    s.remove_prefix(1);
    return string_to_shipnum(s);
  }

  if (s.size() > 0 && std::isdigit(s[0])) {
    return (std::stoi(std::string(s.begin(), s.end())));
  }
  return {};
}

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
      getstar(&Stars[g.snum], g.snum); /*Stars doesn't need to be freed */
      return Stars[g.snum]->ships;
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

int enufAP(int Playernum, int Governor, unsigned short AP, int x) {
  int blah;

  if ((blah = (AP < x))) {
    sprintf(buf, "You don't have %d action points there.\n", x);
    notify(Playernum, Governor, buf);
  }
  return (!blah);
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
    if (racepass == race->password) {
      for (governor_t j = 0; j <= MAXGOVERNORS; j++) {
        if (*race->governor[j].password &&
            govpass == race->governor[j].password) {
          return {race->Playernum, j};
        }
      }
    }
  }
  return {0, 0};
}

/* returns player # from string containing that players name or #. */
player_t get_player(const std::string &name) {
  player_t rnum = 0;

  if (isdigit(name[0])) {
    if ((rnum = std::stoi(name)) < 1 || rnum > Num_races) return 0;
    return rnum;
  }
  for (player_t i = 1; i <= Num_races; i++)
    if (name == races[i - 1]->name) return i;
  return 0;
}

void allocateAPs(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  int maxalloc;
  int alloc;

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
  maxalloc = MIN(Sdata.AP[Playernum - 1],
                 LIMIT_APs - Stars[g.snum]->AP[Playernum - 1]);
  if (alloc > maxalloc) {
    sprintf(buf, "Illegal value (%d) - maximum = %d\n", alloc, maxalloc);
    notify(Playernum, Governor, buf);
    return;
  }
  Sdata.AP[Playernum - 1] -= alloc;
  putsdata(&Sdata);
  getstar(&Stars[g.snum], g.snum);
  Stars[g.snum]->AP[Playernum - 1] =
      MIN(LIMIT_APs, Stars[g.snum]->AP[Playernum - 1] + alloc);
  putstar(Stars[g.snum], g.snum);
  sprintf(buf, "Allocated\n");
  notify(Playernum, Governor, buf);
}

void deductAPs(const player_t Playernum, const governor_t Governor,
               unsigned int n, starnum_t snum, int sdata) {
  if (n) {
    if (!sdata) {
      getstar(&Stars[snum], snum);

      if (Stars[snum]->AP[Playernum - 1] >= n)
        Stars[snum]->AP[Playernum - 1] -= n;
      else {
        Stars[snum]->AP[Playernum - 1] = 0;
        sprintf(buf,
                "WHOA!  You cheater!  Oooohh!  OOOOH!\n  I'm "
                "tellllllllliiiiiiinnnnnnnnnggggggggg!!!!!!!\n");
        notify(Playernum, Governor, buf);
      }

      putstar(Stars[snum], snum);
    } else {
      getsdata(&Sdata);
      Sdata.AP[Playernum - 1] = std::max(0u, Sdata.AP[Playernum - 1] - n);
      putsdata(&Sdata);
    }
  }
}

double morale_factor(double x) {
  return (atan((double)x / 10000.) / 3.14159565 + .5);
}
