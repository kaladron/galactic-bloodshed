// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 *  Getplace -- returns directory level from string and game object
 *  Dispplace -- returns string from directory level
 *  testship(ship) -- tests various things for the ship.
 */

#include "getplace.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "races.h"
#include "ships.h"
#include "shlmisc.h"
#include "tweakables.h"
#include "vars.h"

static char Disps[PLACENAMESIZE];

static placetype Getplace2(int Playernum, int Governor, const char *string,
                           placetype *where, int ignoreexpl, int God);

placetype Getplace(GameObj &g, const std::string &str, const int ignoreexpl) {
  return Getplace(g, str.c_str(), ignoreexpl);
}

placetype Getplace(GameObj &g, const char *const string, const int ignoreexpl) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  placetype where; /* return value */
  racetype *Race;
  int God;

  Race = races[Playernum - 1];
  God = Race->God;

  where.err = 0;

  if (string != nullptr) {
    switch (*string) {
      case '/':
        where.level = ScopeLevel::LEVEL_UNIV; /* scope = root (universe) */
        where.snum = 0;
        where.pnum = where.shipno = 0;
        return (Getplace2(Playernum, Governor, string + 1, &where, ignoreexpl,
                          God));
      case '#':
        sscanf(string + 1, "%ld", &where.shipno);
        if (!getship(&where.shipptr, where.shipno)) {
          DontOwnErr(Playernum, Governor, where.shipno);
          where.err = 1;
          return where;
        }
        if ((where.shipptr->owner == Playernum || ignoreexpl || God) &&
            (where.shipptr->alive || God)) {
          where.level = ScopeLevel::LEVEL_SHIP;
          where.snum = where.shipptr->storbits;
          where.pnum = where.shipptr->pnumorbits;
          free(where.shipptr);
          return where;
        } else {
          where.err = 1;
          free(where.shipptr);
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
  if (string != nullptr && *string == CHAR_CURR_SCOPE)
    return where;
  else
    return Getplace2(Playernum, Governor, string, &where, ignoreexpl, God);
}

static placetype Getplace2(const int Playernum, const int Governor,
                           const char *string, placetype *where,
                           const int ignoreexpl, const int God) {
  char substr[NAMESIZE];
  uint8_t i;
  size_t l;
  int tick;

  if (where->err || string == nullptr || *string == '\0' || *string == '\n')
    return (*where); /* base cases */
  else if (*string == '.') {
    if (where->level == ScopeLevel::LEVEL_UNIV) {
      sprintf(buf, "Can't go higher.\n");
      notify(Playernum, Governor, buf);
      where->err = 1;
      return (*where);
    } else {
      if (where->level == ScopeLevel::LEVEL_SHIP) {
        (void)getship(&where->shipptr, where->shipno);
        where->level = where->shipptr->whatorbits;
        /* Fix 'cs .' for ships within ships. Maarten */
        if (where->level == ScopeLevel::LEVEL_SHIP)
          where->shipno = where->shipptr->destshipno;
        free(where->shipptr);
      } else if (where->level == ScopeLevel::LEVEL_STAR) {
        where->level = ScopeLevel::LEVEL_UNIV;
      } else if (where->level == ScopeLevel::LEVEL_PLAN) {
        where->level = ScopeLevel::LEVEL_STAR;
      }
      while (*string == '.') string++;
      while (*string == '/') string++;
      return (Getplace2(Playernum, Governor, string, where, ignoreexpl, God));
    }
  } else {
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
            return (Getplace2(Playernum, Governor, string + tick, where,
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
          const auto &p = getplanet(where->snum, i);
          if (ignoreexpl || p.info[Playernum - 1].explored || God) {
            tick = (*string == '/');
            return (Getplace2(Playernum, Governor, string + tick, where,
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

std::string Dispplace(const placetype &where) {
  std::ostringstream out;
  switch (where.level) {
    case ScopeLevel::LEVEL_STAR:
      out << "/" << Stars[where.snum]->name;
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      out << "/" << Stars[where.snum]->name << "/"
          << Stars[where.snum]->pnames[where.pnum];
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << where.shipno;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      return out.str();
  }
}

int testship(int Playernum, int Governor, Ship *s) {
  int r;

  r = 0;

  if (!s->alive) {
    sprintf(buf, "%s has been destroyed.\n", ship_to_string(*s).c_str());
    notify(Playernum, Governor, buf);
    r = 1;
  } else if (s->owner != Playernum || !authorized(Governor, s)) {
    DontOwnErr(Playernum, Governor, s->number);
    r = 1;
  } else {
    if (!s->active) {
      sprintf(buf, "%s is irradiated %d%% and inactive.\n",
              ship_to_string(*s).c_str(), s->rad);
      notify(Playernum, Governor, buf);
      r = 1;
    }
    /*   if (!s->popn && s->max_crew) {
         sprintf(buf,"%s has no crew and is not a robotic ship.\n", Ship(s));
         notify(Playernum, Governor, buf);
         r = 1;
         } */
  }
  return r;
}
