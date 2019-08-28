// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 *  getplace -- returns directory level from string and game object
 *  Dispplace -- returns string from directory level
 *  testship(ship) -- tests various things for the ship.
 */

#include "gb/getplace.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static char Disps[PLACENAMESIZE];

static Place getplace2(int Playernum, int Governor, const char *string,
                       Place *where, int ignoreexpl, int God);

Place getplace(GameObj &g, const std::string &string, const int ignoreexpl) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  Place where; /* return value */

  const auto God = races[Playernum - 1]->God;

  where.err = 0;

  if (string.size() != 0) {
    switch (string[0]) {
      case '/':
        where.level = ScopeLevel::LEVEL_UNIV; /* scope = root (universe) */
        where.snum = 0;
        where.pnum = where.shipno = 0;
        return (getplace2(Playernum, Governor, string.c_str() + 1, &where,
                          ignoreexpl, God));
      case '#': {
        auto shipnotmp = string_to_shipnum(string);
        if (shipnotmp)
          where.shipno = *shipnotmp;
        else
          where.shipno = -1;

        auto ship = getship(where.shipno);
        if (!ship) {
          DontOwnErr(Playernum, Governor, where.shipno);
          where.err = 1;
          return where;
        }
        if ((ship->owner == Playernum || ignoreexpl || God) &&
            (ship->alive || God)) {
          where.level = ScopeLevel::LEVEL_SHIP;
          where.snum = ship->storbits;
          where.pnum = ship->pnumorbits;
          return where;
        }
        where.err = 1;
        return where;
      }
      case '-':
        /* no destination */
        where.level = ScopeLevel::LEVEL_UNIV;
        return where;
    }
  }

  /* copy current scope to scope */
  where.level = g.level;
  where.snum = g.snum;
  where.pnum = g.pnum;
  if (where.level == ScopeLevel::LEVEL_SHIP) where.shipno = g.shipno;
  if (string.size() != 0 && string[0] == CHAR_CURR_SCOPE) return where;

  return getplace2(Playernum, Governor, string.c_str(), &where, ignoreexpl,
                   God);
}

static Place getplace2(const int Playernum, const int Governor,
                       const char *string, Place *where, const int ignoreexpl,
                       const int God) {
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

char *Dispshiploc_brief(Ship *ship) {
  int i;

  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_STAR:
      sprintf(Disps, "/%-4.4s", Stars[ship->storbits]->name);
      return (Disps);
    case ScopeLevel::LEVEL_PLAN:
      sprintf(Disps, "/%s", Stars[ship->storbits]->name);
      for (i = 2; (Disps[i] && (i < 5)); i++)
        ;
      sprintf(Disps + i, "/%-4.4s",
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      return (Disps);
    case ScopeLevel::LEVEL_SHIP:
      sprintf(Disps, "#%lu", ship->destshipno);
      return (Disps);
    case ScopeLevel::LEVEL_UNIV:
      sprintf(Disps, "/");
      return (Disps);
  }
}

char *Dispshiploc(Ship *ship) {
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_STAR:
      sprintf(Disps, "/%s", Stars[ship->storbits]->name);
      return (Disps);
    case ScopeLevel::LEVEL_PLAN:
      sprintf(Disps, "/%s/%s", Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      return (Disps);
    case ScopeLevel::LEVEL_SHIP:
      sprintf(Disps, "#%lu", ship->destshipno);
      return (Disps);
    case ScopeLevel::LEVEL_UNIV:
      sprintf(Disps, "/");
      return (Disps);
  }
}

bool testship(const player_t playernum, const governor_t governor,
              const Ship &s) {
  if (!s.alive) {
    sprintf(buf, "%s has been destroyed.\n", ship_to_string(s).c_str());
    notify(playernum, governor, buf);
    return true;
  }

  if (s.owner != playernum || !authorized(governor, s)) {
    DontOwnErr(playernum, governor, s.number);
    return true;
  }

  if (!s.active) {
    sprintf(buf, "%s is irradiated %d%% and inactive.\n",
            ship_to_string(s).c_str(), s.rad);
    notify(playernum, governor, buf);
    return true;
  }

  return false;
}
