// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  miscellaneous stuff included in the shell */

#include "shlmisc.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#include "GB_server.h"
#include "buffers.h"
#include "files.h"
#include "files_shl.h"
#include "max.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

#include <boost/format.hpp>

static void do_revoke(racetype *, const governor_t, const governor_t);

// TODO(jeffbailey): Move this into the ship class when we stop using bzero to
// initalize it.
std::string Ship(const ship &s) {
  return str(boost::format("%c%lu %s [%d]") % Shipltrs[s.type] % s.number %
             s.name % s.owner);
}

void grant(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // int APcount = 0; TODO(jeffbailey);
  racetype *Race;
  governor_t gov;
  shipnum_t nextshipno, shipno;
  shiptype *ship;

  Race = races[Playernum - 1];
  if (argv.size() < 3) {
    g.out << "Syntax: grant <governor> star\n";
    g.out << "        grant <governor> ship <shiplist>\n";
    g.out << "        grant <governor> money <amount>\n";
    return;
  }
  if ((gov = atoi(argv[1].c_str())) > MAXGOVERNORS) {
    g.out << "Bad governor number.\n";
    return;
  } else if (!Race->governor[gov].active) {
    g.out << "That governor is not active.\n";
    return;
  } else if (argv[2] == "star") {
    int snum;
    if (g.level != ScopeLevel::LEVEL_STAR) {
      g.out << "Please cs to the star system first.\n";
      return;
    }
    snum = g.snum;
    Stars[snum]->governor[Playernum - 1] = gov;
    sprintf(buf, "\"%s\" has granted you control of the /%s star system.\n",
            Race->governor[Governor].name, Stars[snum]->name);
    warn(Playernum, gov, buf);
    putstar(Stars[snum], snum);
  } else if (argv[2] == "ship") {
    nextshipno = start_shiplist(g, argv[3].c_str());
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (in_list(Playernum, argv[3].c_str(), ship, &nextshipno) &&
          authorized(Governor, ship)) {
        ship->governor = gov;
        sprintf(buf, "\"%s\" granted you %s at %s\n",
                Race->governor[Governor].name, Ship(*ship).c_str(),
                prin_ship_orbits(ship));
        warn(Playernum, gov, buf);
        putship(ship);
        sprintf(buf, "%s granted to \"%s\"\n", Ship(*ship).c_str(),
                Race->governor[gov].name);
        notify(Playernum, Governor, buf);
        free(ship);
      } else
        free(ship);
  } else if (argv[2] == "money") {
    long amount;
    if (argv.size() < 4) {
      g.out << "Indicate the amount of money.\n";
      return;
    }
    amount = atoi(argv[3].c_str());
    if (amount < 0 && Governor) {
      g.out << "Only leaders may make take away money.\n";
      return;
    }
    if (amount > Race->governor[Governor].money)
      amount = Race->governor[Governor].money;
    else if (-amount > Race->governor[gov].money)
      amount = -Race->governor[gov].money;
    if (amount >= 0)
      sprintf(buf, "%ld money granted to \"%s\".\n", amount,
              Race->governor[gov].name);
    else
      sprintf(buf, "%ld money deducted from \"%s\".\n", -amount,
              Race->governor[gov].name);
    notify(Playernum, Governor, buf);
    if (amount >= 0)
      sprintf(buf, "\"%s\" granted you %ld money.\n",
              Race->governor[Governor].name, amount);
    else
      sprintf(buf, "\"%s\" docked you %ld money.\n",
              Race->governor[Governor].name, -amount);
    warn(Playernum, gov, buf);
    Race->governor[Governor].money -= amount;
    Race->governor[gov].money += amount;
    putrace(Race);
    return;
  } else
    g.out << "You can't grant that.\n";
}

void governors(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  racetype *Race;
  int i;
  int gov;

  Race = races[Playernum - 1];
  if (Governor ||
      argv.size() < 3) { /* the only thing governors can do with this */
    for (i = 0; i <= MAXGOVERNORS; i++) {
      if (Governor)
        sprintf(buf, "%d %-15.15s %8s %10ld %s", i, Race->governor[i].name,
                Race->governor[i].active ? "ACTIVE" : "INACTIVE",
                Race->governor[i].money, ctime(&Race->governor[i].login));
      else
        sprintf(buf, "%d %-15.15s %-10.10s %8s %10ld %s", i,
                Race->governor[i].name, Race->governor[i].password,
                Race->governor[i].active ? "ACTIVE" : "INACTIVE",
                Race->governor[i].money, ctime(&Race->governor[i].login));
      notify(Playernum, Governor, buf);
    }
  } else if ((gov = atoi(argv[1].c_str())) < 0 || gov > MAXGOVERNORS) {
    g.out << "No such governor.\n";
    return;
  } else if (argv[0] == "appoint") {
    /* Syntax: 'appoint <gov> <password>' */
    if (Race->governor[gov].active) {
      g.out << "That governor is already appointed.\n";
      return;
    }
    Race->governor[gov].active = 1;
    Race->governor[gov].homelevel = Race->governor[gov].deflevel =
        Race->governor[0].deflevel;
    Race->governor[gov].homesystem = Race->governor[gov].defsystem =
        Race->governor[0].defsystem;
    Race->governor[gov].homeplanetnum = Race->governor[gov].defplanetnum =
        Race->governor[0].defplanetnum;
    Race->governor[gov].money = 0;
    Race->governor[gov].toggle.highlight = Playernum;
    Race->governor[gov].toggle.inverse = 1;
    strncpy(Race->governor[gov].password, argv[2].c_str(), RNAMESIZE - 1);
    putrace(Race);
    g.out << "Governor activated.\n";
    return;
  } else if (argv[0] == "revoke") {
    int j;
    if (!gov) {
      g.out << "You can't revoke your leadership!\n";
      return;
    }
    if (!Race->governor[gov].active) {
      g.out << "That governor is not active.\n";
      return;
    }
    if (argv.size() < 4)
      j = 0;
    else
      j = atoi(argv[3].c_str()); /* who gets this governors stuff */
    if (j < 0 || j > MAXGOVERNORS) {
      g.out << "You can't give stuff to that governor!\n";
      return;
    }
    if (!strcmp(Race->governor[gov].password, argv[2].c_str())) {
      g.out << "Incorrect password.\n";
      return;
    }
    if (!Race->governor[j].active || j == gov) {
      g.out << "Bad target governor.\n";
      return;
    }
    do_revoke(Race, gov, j); /* give stuff from gov to j */
    putrace(Race);
    g.out << "Done.\n";
    return;
  } else if (argv[2] == "password") {
    if (Race->Guest) {
      g.out << "Guest races cannot change passwords.\n";
      return;
    }
    if (argv.size() < 4) {
      g.out << "You must give a password.\n";
      return;
    }
    if (!Race->governor[gov].active) {
      g.out << "That governor is inactive.\n";
      return;
    }
    strncpy(Race->governor[gov].password, argv[3].c_str(), RNAMESIZE - 1);
    putrace(Race);
    g.out << "Password changed.\n";
    return;
  } else
    g.out << "Bad option.\n";
}

static void do_revoke(racetype *Race, const governor_t src_gov,
                      const governor_t tgt_gov) {
  shiptype *ship;

  std::string outmsg;
  outmsg = str(
      boost::format("*** Transferring [%d,%d]'s ownings to [%d,%d] ***\n\n") %
      Race->Playernum % src_gov % Race->Playernum % tgt_gov);
  notify(Race->Playernum, (governor_t)0, outmsg);

  /*  First do stars....  */

  for (starnum_t i = 0; i < Sdata.numstars; i++)
    if (Stars[i]->governor[Race->Playernum - 1] == src_gov) {
      Stars[i]->governor[Race->Playernum - 1] = tgt_gov;
      outmsg = str(boost::format("Changed juridiction of /%s...\n") %
                   Stars[i]->name);
      notify(Race->Playernum, 0, outmsg);
      putstar(Stars[i], i);
    }

  /*  Now do ships....  */
  Num_ships = Numships();
  for (shipnum_t i = 1; i <= Num_ships; i++) {
    (void)getship(&ship, i);
    if (ship->alive && (ship->owner == Race->Playernum) &&
        (ship->governor == src_gov)) {
      ship->governor = tgt_gov;
      outmsg = str(boost::format("Changed ownership of %c%lu...\n") %
                   Shipltrs[ship->type] % i);
      notify(Race->Playernum, 0, outmsg);
      putship(ship);
    }
    free(ship);
  }

  /*  And money too....  */

  outmsg = str(boost::format("Transferring %ld money...\n") %
               Race->governor[src_gov].money);
  notify(Race->Playernum, 0, outmsg);
  Race->governor[tgt_gov].money =
      Race->governor[tgt_gov].money + Race->governor[src_gov].money;
  Race->governor[src_gov].money = 0;

  /* And last but not least, flag the governor as inactive.... */

  Race->governor[src_gov].active = 0;
  strcpy(Race->governor[src_gov].password, "");
  strcpy(Race->governor[src_gov].name, "");
  outmsg = str(
      boost::format("\n*** Governor [%d,%d]'s powers have been REVOKED ***\n") %
      Race->Playernum % src_gov);
  notify(Race->Playernum, 0, outmsg);

  // TODO(jeffbailey): Use C++17 Filesystem stuff when available
  std::string rm_telegram_file = str(boost::format("rm %s.%d.%d") % TELEGRAMFL %
                                     Race->Playernum % src_gov);
  system(rm_telegram_file.c_str()); /*  Remove the telegram file too....  */

  return;
}

int authorized(int Governor, shiptype *ship) {
  return (!Governor || ship->governor == Governor);
}

int start_shiplist(GameObj &g, const char *p) {
  shiptype *ship;
  int st, pl, sh;

  if (*p == '#') return (atoi(++p));
  if (isdigit(*p)) return (atoi(p));

  /*ship number not given */
  st = g.snum;
  pl = g.pnum;
  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      getsdata(&Sdata);
      return Sdata.ships;
    case ScopeLevel::LEVEL_STAR:
      getstar(&Stars[st], st); /*Stars doesn't need to be freed */
      return Stars[st]->ships;
    case ScopeLevel::LEVEL_PLAN: {
      const auto &planet = getplanet(st, pl);
      sh = planet.ships;
      return sh;
    }
    case ScopeLevel::LEVEL_SHIP:
      (void)getship(&ship, g.shipno);
      sh = ship->ships;
      free(ship);
      return sh;
  }
}

/* Step through linked list at current player scope */
shipnum_t do_shiplist(shiptype **s, shipnum_t *nextshipno) {
  shipnum_t shipno;
  if (!(shipno = *nextshipno)) return 0;

  if (!getship(s, shipno)) /* allocate memory, free in loop */
    return 0;
  *nextshipno = (*s)->nextship;
  return shipno;
}

int in_list(player_t Playernum, const char *list, shiptype *s,
            shipnum_t *nextshipno) {
  const char *p;
  if (s->owner != Playernum || !s->alive) return 0;
  const char q = Shipltrs[s->type];
  p = list;
  if (*p == '#' || isdigit(*p)) {
    if (s->owner != Playernum || !s->alive) return 0;
    *nextshipno = 0;
    return s->number;
  }
  for (; *p; p++)
    if (*p == q || *p == '*') return s->number; /* '*' is a wildcard */
  return 0;
}

/** Deity fix-it utilities */
void fix(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (!g.god) {
    notify(Playernum, Governor,
           "This command is only available to the deity.\n");
    return;
  }

  shiptype *s;

  if (argv[1] == "planet") {
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "Change scope to the planet first.\n";
      return;
    }
    auto p = getplanet(g.snum, g.pnum);
    if (argv[2] == "Maxx") {
      if (argv.size() > 3) p.Maxx = atoi(argv[3].c_str());
      sprintf(buf, "Maxx = %d\n", p.Maxx);
    } else if (argv[2] == "Maxy") {
      if (argv.size() > 3) p.Maxy = atoi(argv[3].c_str());
      sprintf(buf, "Maxy = %d\n", p.Maxy);
    } else if (argv[2] == "xpos") {
      if (argv.size() > 3) p.xpos = (double)atoi(argv[3].c_str());
      sprintf(buf, "xpos = %f\n", p.xpos);
    } else if (argv[2] == "ypos") {
      if (argv.size() > 3) p.ypos = (double)atoi(argv[3].c_str());
      sprintf(buf, "ypos = %f\n", p.ypos);
    } else if (argv[2] == "ships") {
      if (argv.size() > 3) p.ships = atoi(argv[3].c_str());
      sprintf(buf, "ships = %ld\n", p.ships);
    } else if (argv[2] == "rtemp") {
      if (argv.size() > 3) p.conditions[RTEMP] = atoi(argv[3].c_str());
      sprintf(buf, "RTEMP = %d\n", p.conditions[RTEMP]);
    } else if (argv[2] == "temperature") {
      if (argv.size() > 3) p.conditions[TEMP] = atoi(argv[3].c_str());
      sprintf(buf, "TEMP = %d\n", p.conditions[TEMP]);
    } else if (argv[2] == "methane") {
      if (argv.size() > 3) p.conditions[METHANE] = atoi(argv[3].c_str());
      sprintf(buf, "METHANE = %d\n", p.conditions[METHANE]);
    } else if (argv[2] == "oxygen") {
      if (argv.size() > 3) p.conditions[OXYGEN] = atoi(argv[3].c_str());
      sprintf(buf, "OXYGEN = %d\n", p.conditions[OXYGEN]);
    } else if (argv[2] == "co2") {
      if (argv.size() > 3) p.conditions[CO2] = atoi(argv[3].c_str());
      sprintf(buf, "CO2 = %d\n", p.conditions[CO2]);
    } else if (argv[2] == "hydrogen") {
      if (argv.size() > 3) p.conditions[HYDROGEN] = atoi(argv[3].c_str());
      sprintf(buf, "HYDROGEN = %d\n", p.conditions[HYDROGEN]);
    } else if (argv[2] == "nitrogen") {
      if (argv.size() > 3) p.conditions[NITROGEN] = atoi(argv[3].c_str());
      sprintf(buf, "NITROGEN = %d\n", p.conditions[NITROGEN]);
    } else if (argv[2] == "sulfur") {
      if (argv.size() > 3) p.conditions[SULFUR] = atoi(argv[3].c_str());
      sprintf(buf, "SULFUR = %d\n", p.conditions[SULFUR]);
    } else if (argv[2] == "helium") {
      if (argv.size() > 3) p.conditions[HELIUM] = atoi(argv[3].c_str());
      sprintf(buf, "HELIUM = %d\n", p.conditions[HELIUM]);
    } else if (argv[2] == "other") {
      if (argv.size() > 3) p.conditions[OTHER] = atoi(argv[3].c_str());
      sprintf(buf, "OTHER = %d\n", p.conditions[OTHER]);
    } else if (argv[2] == "toxic") {
      if (argv.size() > 3) p.conditions[TOXIC] = atoi(argv[3].c_str());
      sprintf(buf, "TOXIC = %d\n", p.conditions[TOXIC]);
    } else {
      g.out << "No such option for 'fix planet'.\n";
      return;
    }
    notify(Playernum, Governor, buf);
    if (argv.size() > 3) putplanet(p, Stars[g.snum], g.pnum);
    return;
  }
  if (argv[1] == "ship") {
    if (g.level != ScopeLevel::LEVEL_SHIP) {
      notify(Playernum, Governor,
             "Change scope to the ship you wish to fix.\n");
      return;
    }
    (void)getship(&s, g.shipno);
    if (argv[2] == "fuel") {
      if (argv.size() > 3) s->fuel = (double)atoi(argv[3].c_str());
      sprintf(buf, "fuel = %f\n", s->fuel);
    } else if (argv[2] == "max_fuel") {
      if (argv.size() > 3) s->max_fuel = atoi(argv[3].c_str());
      sprintf(buf, "fuel = %d\n", s->max_fuel);
    } else if (argv[2] == "destruct") {
      if (argv.size() > 3) s->destruct = atoi(argv[3].c_str());
      sprintf(buf, "destruct = %d\n", s->destruct);
    } else if (argv[2] == "resource") {
      if (argv.size() > 3) s->resource = atoi(argv[3].c_str());
      sprintf(buf, "resource = %d\n", s->resource);
    } else if (argv[2] == "damage") {
      if (argv.size() > 3) s->damage = atoi(argv[3].c_str());
      sprintf(buf, "damage = %d\n", s->damage);
    } else if (argv[2] == "alive") {
      s->alive = 1;
      s->damage = 0;
      sprintf(buf, "%s resurrected\n", Ship(*s).c_str());
    } else if (argv[2] == "dead") {
      s->alive = 0;
      s->damage = 100;
      sprintf(buf, "%s destroyed\n", Ship(*s).c_str());
    } else {
      g.out << "No such option for 'fix ship'.\n";
      free(s);
      return;
    }
    notify(Playernum, Governor, buf);
    putship(s);
    free(s);
    return;
  } else
    g.out << "Fix what?\n";
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

void Getracenum(char *racepass, char *govpass, int *racenum, int *govnum) {
  int i, j;
  for (i = 1; i <= Num_races; i++) {
    if (!strcmp(racepass, races[i - 1]->password)) {
      *racenum = i;
      for (j = 0; j <= MAXGOVERNORS; j++) {
        if (*races[i - 1]->governor[j].password &&
            !strcmp(govpass, races[i - 1]->governor[j].password)) {
          *govnum = j;
          return;
        }
      }
    }
  }
  *racenum = *govnum = 0;
}

/* returns player # from string containing that players name or #. */
player_t get_player(const std::string &name) {
  player_t rnum = 0;

  if (isdigit(name[0])) {
    if ((rnum = std::stoi(name)) < 1 || rnum > Num_races) return 0;
    return rnum;
  } else {
    for (player_t i = 1; i <= Num_races; i++)
      if (name == races[i - 1]->name) return i;
    return 0;
  }
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
  alloc = atoi(argv[1].c_str());
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
