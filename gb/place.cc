// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/place.h"

#include <cstring>
#include <sstream>
#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

namespace {
Place getplace2(const int Playernum, const int Governor, const char* string,
                Place* where, const int ignoreexpl, const int God) {
  char substr[NAMESIZE];
  uint8_t i;
  size_t l;
  int tick;

  if (where->err || string == nullptr || *string == '\0' || *string == '\n')
    return (*where); /* base cases */
  if (*string == '.') {
    if (where->level == ScopeLevel::LEVEL_UNIV) {
      sprintf(buf, "Can't go higher.\n");
      notify(Playernum, Governor, buf);
      where->err = 1;
      return (*where);
    }
    if (where->level == ScopeLevel::LEVEL_SHIP) {
      auto ship = getship(where->shipno);
      where->level = ship->whatorbits;
      /* Fix 'cs .' for ships within ships. Maarten */
      if (where->level == ScopeLevel::LEVEL_SHIP)
        where->shipno = ship->destshipno;
    } else if (where->level == ScopeLevel::LEVEL_STAR) {
      where->level = ScopeLevel::LEVEL_UNIV;
    } else if (where->level == ScopeLevel::LEVEL_PLAN) {
      where->level = ScopeLevel::LEVEL_STAR;
    }
    while (*string == '.') string++;
    while (*string == '/') string++;
    return (getplace2(Playernum, Governor, string, where, ignoreexpl, God));
  }
  /* is a char string, name of something */
  sscanf(string, "%[^/ \n]", substr);
  do {
    /*if (isupper(*string) )
      (*string) = tolower(*string);*/
    string++;
  } while (*string != '/' && *string != '\n' && *string != '\0');
  l = strlen(substr);
  if (where->level == ScopeLevel::LEVEL_UNIV) {
    for (i = 0; i < Sdata.numstars; i++)
      if (!strncmp(substr, Stars[i]->name, l)) {
        where->level = ScopeLevel::LEVEL_STAR;
        where->snum = i;
        if (ignoreexpl || isset(Stars[where->snum]->explored, Playernum) ||
            God) {
          tick = (*string == '/');
          return (getplace2(Playernum, Governor, string + tick, where,
                            ignoreexpl, God));
        }
        sprintf(buf, "You have not explored %s yet.\n",
                Stars[where->snum]->name);
        notify(Playernum, Governor, buf);
        where->err = 1;
        return (*where);
      }
    if (i >= Sdata.numstars) {
      sprintf(buf, "No such star %s.\n", substr);
      notify(Playernum, Governor, buf);
      where->err = 1;
      return (*where);
    }
  } else if (where->level == ScopeLevel::LEVEL_STAR) {
    for (i = 0; i < Stars[where->snum]->numplanets; i++)
      if (!strncmp(substr, Stars[where->snum]->pnames[i], l)) {
        where->level = ScopeLevel::LEVEL_PLAN;
        where->pnum = i;
        const auto p = getplanet(where->snum, i);
        if (ignoreexpl || p.info[Playernum - 1].explored || God) {
          tick = (*string == '/');
          return (getplace2(Playernum, Governor, string + tick, where,
                            ignoreexpl, God));
        }
        sprintf(buf, "You have not explored %s yet.\n",
                Stars[where->snum]->pnames[i]);
        notify(Playernum, Governor, buf);
        where->err = 1;
        return (*where);
      }
    if (i >= Stars[where->snum]->numplanets) {
      sprintf(buf, "No such planet %s.\n", substr);
      notify(Playernum, Governor, buf);
      where->err = 1;
      return (*where);
    }
  } else {
    sprintf(buf, "Can't descend to %s.\n", substr);
    notify(Playernum, Governor, buf);
    where->err = 1;
    return (*where);
  }

  return (*where);
}
}  // Namespace

Place::Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_)
    : level(level_), snum(snum_), pnum(pnum_), shipno(0) {
  if (level_ == ScopeLevel::LEVEL_SHIP)
    throw std::runtime_error("Must give ship number when level is a ship");
}

std::string Place::to_string() {
  std::ostringstream out;
  switch (level) {
    case ScopeLevel::LEVEL_STAR:
      out << "/" << Stars[snum]->name;
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      out << "/" << Stars[snum]->name << "/" << Stars[snum]->pnames[pnum];
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << shipno;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      return out.str();
  }
}

Place::Place(GameObj& g, const std::string& string, const bool ignoreexpl) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  const auto God = races[Playernum - 1]->God;

  err = 0;

  if (string.size() != 0) {
    switch (string[0]) {
      case '/':
        level = ScopeLevel::LEVEL_UNIV; /* scope = root (universe) */
        snum = 0;
        pnum = shipno = 0;
        getplace2(Playernum, Governor, string.c_str() + 1, this, ignoreexpl,
                  God);
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
        if ((ship->owner == Playernum || ignoreexpl || God) &&
            (ship->alive || God)) {
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

  getplace2(Playernum, Governor, string.c_str(), this, ignoreexpl, God);
}
