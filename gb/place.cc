// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/place.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void Place::getplace2(GameObj& g, std::string_view string,
                      const bool ignoreexpl) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  if (err || string.empty()) return;

  if (string.starts_with('.')) {
    if (level == ScopeLevel::LEVEL_UNIV) {
      g.out << "Can't go higher.\n";
      err = true;
      return;
    }
    if (level == ScopeLevel::LEVEL_SHIP) {
      auto ship = getship(shipno);
      level = ship->whatorbits;
      /* Fix 'cs .' for ships within ships. Maarten */
      if (level == ScopeLevel::LEVEL_SHIP) shipno = ship->destshipno;
    } else if (level == ScopeLevel::LEVEL_STAR) {
      level = ScopeLevel::LEVEL_UNIV;
    } else if (level == ScopeLevel::LEVEL_PLAN) {
      level = ScopeLevel::LEVEL_STAR;
    }
    while (string.starts_with('.')) string.remove_prefix(1);
    while (string.starts_with('/')) string.remove_prefix(1);
    return getplace2(g, string, ignoreexpl);
  }

  // It's the name of something
  std::string_view substr = string.substr(0, string.find_first_of("/"));
  do {
    string.remove_prefix(1);
  } while (!string.starts_with('/') && !string.empty());

  switch (level) {
    case ScopeLevel::LEVEL_UNIV:
      for (auto i = 0; i < Sdata.numstars; i++)
        if (substr == stars[i].name) {
          level = ScopeLevel::LEVEL_STAR;
          snum = i;
          if (ignoreexpl || isset(stars[snum].explored, Playernum) || g.god) {
            if (string.starts_with('/')) string.remove_prefix(1);
            return getplace2(g, string, ignoreexpl);
          }
          sprintf(buf, "You have not explored %s yet.\n", stars[snum].name);
          notify(Playernum, Governor, buf);
          err = true;
          return;
        }
      sprintf(buf, "No such star %s.\n", substr.data());
      notify(Playernum, Governor, buf);
      err = true;
      return;
    case ScopeLevel::LEVEL_STAR:
      for (auto i = 0; i < stars[snum].numplanets; i++)
        if (substr == stars[snum].pnames[i]) {
          level = ScopeLevel::LEVEL_PLAN;
          pnum = i;
          const auto p = getplanet(snum, i);
          if (ignoreexpl || p.info[Playernum - 1].explored || g.god) {
            if (string.starts_with('/')) string.remove_prefix(1);
            return getplace2(g, string, ignoreexpl);
          }
          sprintf(buf, "You have not explored %s yet.\n",
                  stars[snum].pnames[i]);
          notify(Playernum, Governor, buf);
          err = true;
          return;
        }
      sprintf(buf, "No such planet %s.\n", substr.data());
      notify(Playernum, Governor, buf);
      err = true;
      return;
    default:
      sprintf(buf, "Can't descend to %s.\n", substr.data());
      notify(Playernum, Governor, buf);
      err = true;
      return;
  }
}

Place::Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_)
    : level(level_), snum(snum_), pnum(pnum_), shipno(0) {
  if (level_ == ScopeLevel::LEVEL_SHIP) err = true;
}

std::string Place::to_string() {
  std::ostringstream out;
  switch (level) {
    case ScopeLevel::LEVEL_STAR:
      out << "/" << stars[snum].name;
      out << std::ends;
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      out << "/" << stars[snum].name << "/" << stars[snum].pnames[pnum];
      out << std::ends;
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << shipno;
      out << std::ends;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      out << std::ends;
      return out.str();
  }
}

Place::Place(GameObj& g, const std::string& string, const bool ignoreexpl) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  err = 0;

  if (string.size() != 0) {
    switch (string[0]) {
      case '/':
        level = ScopeLevel::LEVEL_UNIV; /* scope = root (universe) */
        snum = 0;
        pnum = shipno = 0;
        getplace2(g, string.c_str() + 1, ignoreexpl);
        return;
      case '#': {
        auto shipnotmp = string_to_shipnum(string);
        if (shipnotmp)
          shipno = *shipnotmp;
        else
          shipno = -1;

        auto ship = getship(shipno);
        if (!ship) {
          DontOwnErr(Playernum, Governor, shipno);
          err = 1;
          return;
        }
        if ((ship->owner == Playernum || ignoreexpl || g.god) &&
            (ship->alive || g.god)) {
          level = ScopeLevel::LEVEL_SHIP;
          snum = ship->storbits;
          pnum = ship->pnumorbits;
          return;
        }
        err = 1;
        return;
      }
      case '-':
        /* no destination */
        level = ScopeLevel::LEVEL_UNIV;
        return;
    }
  }

  /* copy current scope to scope */
  level = g.level;
  snum = g.snum;
  pnum = g.pnum;
  if (level == ScopeLevel::LEVEL_SHIP) shipno = g.shipno;
  if (string.size() != 0 && string[0] == CHAR_CURR_SCOPE) return;

  getplace2(g, string.c_str(), ignoreexpl);
}
