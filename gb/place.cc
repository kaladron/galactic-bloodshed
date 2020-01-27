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

void Place::getplace2(GameObj& g, const char* string, const bool ignoreexpl) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  char substr[NAMESIZE];
  uint8_t i;
  size_t l;
  int tick;

  if (err || string == nullptr || *string == '\0' || *string == '\n')
    return; /* base cases */
  if (*string == '.') {
    if (level == ScopeLevel::LEVEL_UNIV) {
      sprintf(buf, "Can't go higher.\n");
      notify(Playernum, Governor, buf);
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
    while (*string == '.') string++;
    while (*string == '/') string++;
    return getplace2(g, string, ignoreexpl);
  }
  /* is a char string, name of something */
  sscanf(string, "%[^/ \n]", substr);
  do {
    /*if (isupper(*string) )
      (*string) = tolower(*string);*/
    string++;
  } while (*string != '/' && *string != '\n' && *string != '\0');
  l = strlen(substr);
  if (level == ScopeLevel::LEVEL_UNIV) {
    for (i = 0; i < Sdata.numstars; i++)
      if (!strncmp(substr, stars[i].name, l)) {
        level = ScopeLevel::LEVEL_STAR;
        snum = i;
        if (ignoreexpl || isset(stars[snum].explored, Playernum) || g.god) {
          tick = (*string == '/');
          return getplace2(g, string + tick, ignoreexpl);
        }
        sprintf(buf, "You have not explored %s yet.\n", stars[snum].name);
        notify(Playernum, Governor, buf);
        err = true;
        return;
      }
    if (i >= Sdata.numstars) {
      sprintf(buf, "No such star %s.\n", substr);
      notify(Playernum, Governor, buf);
      err = true;
      return;
    }
  } else if (level == ScopeLevel::LEVEL_STAR) {
    for (i = 0; i < stars[snum].numplanets; i++)
      if (!strncmp(substr, stars[snum].pnames[i], l)) {
        level = ScopeLevel::LEVEL_PLAN;
        pnum = i;
        const auto p = getplanet(snum, i);
        if (ignoreexpl || p.info[Playernum - 1].explored || g.god) {
          tick = (*string == '/');
          return getplace2(g, string + tick, ignoreexpl);
        }
        sprintf(buf, "You have not explored %s yet.\n", stars[snum].pnames[i]);
        notify(Playernum, Governor, buf);
        err = true;
        return;
      }
    if (i >= stars[snum].numplanets) {
      sprintf(buf, "No such planet %s.\n", substr);
      notify(Playernum, Governor, buf);
      err = true;
      return;
    }
  } else {
    sprintf(buf, "Can't descend to %s.\n", substr);
    notify(Playernum, Governor, buf);
    err = true;
    return;
  }

  return;
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
