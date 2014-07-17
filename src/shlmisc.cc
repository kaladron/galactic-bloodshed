// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  miscellaneous stuff included in the shell */

#include "shlmisc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "GB_server.h"
#include "buffers.h"
#include "files.h"
#include "files_shl.h"
#include "max.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

char *Ship(shiptype *s) {
  adr = !adr; /* switch between 0 and 1 - adr is a global variable */
  sprintf(junk[adr], "%c%d %s [%d]", Shipltrs[s->type], s->number, s->name,
          s->owner);
  return junk[adr]; /* junk is a global buffer */
}

void grant(int Playernum, int Governor, int APcount) {
  racetype *Race;
  int gov, nextshipno, shipno;
  shiptype *ship;

  Race = races[Playernum - 1];
  if (argn < 3) {
    notify(Playernum, Governor, "Syntax: grant <governor> star\n");
    notify(Playernum, Governor, "        grant <governor> ship <shiplist>\n");
    notify(Playernum, Governor, "        grant <governor> money <amount>\n");
    return;
  }
  if ((gov = atoi(args[1])) < 0 || gov > MAXGOVERNORS) {
    notify(Playernum, Governor, "Bad governor number.\n");
    return;
  } else if (!Race->governor[gov].active) {
    notify(Playernum, Governor, "That governor is not active.\n");
    return;
  } else if (match(args[2], "star")) {
    int snum;
    if (Dir[Playernum - 1][Governor].level != LEVEL_STAR) {
      notify(Playernum, Governor, "Please cs to the star system first.\n");
      return;
    }
    snum = Dir[Playernum - 1][Governor].snum;
    Stars[snum]->governor[Playernum - 1] = gov;
    sprintf(buf, "\"%s\" has granted you control of the /%s star system.\n",
            Race->governor[Governor].name, Stars[snum]->name);
    warn(Playernum, gov, buf);
    putstar(Stars[snum], snum);
  } else if (match(args[2], "ship")) {
    nextshipno = start_shiplist(Playernum, Governor, args[3]);
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (in_list(Playernum, args[3], ship, &nextshipno) &&
          authorized(Governor, ship)) {
        ship->governor = gov;
        sprintf(buf, "\"%s\" granted you %s at %s\n",
                Race->governor[Governor].name, Ship(ship),
                prin_ship_orbits(ship));
        warn(Playernum, gov, buf);
        putship(ship);
        sprintf(buf, "%s granted to \"%s\"\n", Ship(ship),
                Race->governor[gov].name);
        notify(Playernum, Governor, buf);
        free(ship);
      } else
        free(ship);
  } else if (match(args[2], "money")) {
    int amount;
    if (argn < 4) {
      notify(Playernum, Governor, "Indicate the amount of money.\n");
      return;
    }
    amount = atoi(args[3]);
    if (amount < 0 && Governor) {
      notify(Playernum, Governor, "Only leaders may make take away money.\n");
      return;
    }
    if (amount > Race->governor[Governor].money)
      amount = Race->governor[Governor].money;
    else if (-amount > Race->governor[gov].money)
      amount = -Race->governor[gov].money;
    if (amount >= 0)
      sprintf(buf, "%d money granted to \"%s\".\n", amount,
              Race->governor[gov].name);
    else
      sprintf(buf, "%d money deducted from \"%s\".\n", -amount,
              Race->governor[gov].name);
    notify(Playernum, Governor, buf);
    if (amount >= 0)
      sprintf(buf, "\"%s\" granted you %d money.\n",
              Race->governor[Governor].name, amount);
    else
      sprintf(buf, "\"%s\" docked you %d money.\n",
              Race->governor[Governor].name, -amount);
    warn(Playernum, gov, buf);
    Race->governor[Governor].money -= amount;
    Race->governor[gov].money += amount;
    putrace(Race);
    return;
  } else
    notify(Playernum, Governor, "You can't grant that.\n");
}

void governors(int Playernum, int Governor, int APcount) {
  racetype *Race;
  reg int i;
  int gov;

  Race = races[Playernum - 1];
  if (Governor || argn < 3) { /* the only thing governors can do with this */
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
  } else if ((gov = atoi(args[1])) < 0 || gov > MAXGOVERNORS) {
    notify(Playernum, Governor, "No such governor.\n");
    return;
  } else if (match(args[0], "appoint")) {
    /* Syntax: 'appoint <gov> <password>' */
    if (Race->governor[gov].active) {
      notify(Playernum, Governor, "That governor is already appointed.\n");
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
    strncpy(Race->governor[gov].password, args[2], RNAMESIZE - 1);
    putrace(Race);
    notify(Playernum, Governor, "Governor activated.\n");
    return;
  } else if (match(args[0], "revoke")) {
    reg int j;
    if (!gov) {
      notify(Playernum, Governor, "You can't revoke your leadership!\n");
      return;
    }
    if (!Race->governor[gov].active) {
      notify(Playernum, Governor, "That governor is not active.\n");
      return;
    }
    if (argn < 4)
      j = 0;
    else
      j = atoi(args[3]); /* who gets this governors stuff */
    if (j < 0 || j > MAXGOVERNORS) {
      notify(Playernum, Governor, "You can't give stuff to that governor!\n");
      return;
    }
    if (!strcmp(Race->governor[gov].password, args[2])) {
      notify(Playernum, Governor, "Incorrect password.\n");
      return;
    }
    if (!Race->governor[j].active || j == gov) {
      notify(Playernum, Governor, "Bad target governor.\n");
      return;
    }
    do_revoke(Race, gov, j); /* give stuff from gov to j */
    putrace(Race);
    notify(Playernum, Governor, "Done.\n");
    return;
  } else if (match(args[2], "password")) {
    if (Race->Guest) {
      notify(Playernum, Governor, "Guest races cannot change passwords.\n");
      return;
    }
    if (argn < 4) {
      notify(Playernum, Governor, "You must give a password.\n");
      return;
    }
    if (!Race->governor[gov].active) {
      notify(Playernum, Governor, "That governor is inactive.\n");
      return;
    }
    strncpy(Race->governor[gov].password, args[3], RNAMESIZE - 1);
    putrace(Race);
    notify(Playernum, Governor, "Password changed.\n");
    return;
  } else
    notify(Playernum, Governor, "Bad option.\n");
}

void do_revoke(racetype *Race, int gov, int j) {
  register int i;
  char revoke_buf[1024];
  shiptype *ship;

  sprintf(revoke_buf, "*** Transferring [%d,%d]'s ownings to [%d,%d] ***\n\n",
          Race->Playernum, gov, Race->Playernum, j);
  notify(Race->Playernum, 0, revoke_buf);

  /*  First do stars....  */

  for (i = 0; i < Sdata.numstars; i++)
    if (Stars[i]->governor[Race->Playernum - 1] == gov) {
      Stars[i]->governor[Race->Playernum - 1] = j;
      sprintf(revoke_buf, "Changed juridiction of /%s...\n", Stars[i]->name);
      notify(Race->Playernum, 0, revoke_buf);
      putstar(Stars[i], i);
    }

  /*  Now do ships....  */
  Num_ships = Numships();
  for (i = 1; i <= Num_ships; i++) {
    (void)getship(&ship, i);
    if (ship->alive && (ship->owner == Race->Playernum) &&
        (ship->governor == gov)) {
      ship->governor = j;
      sprintf(revoke_buf, "Changed ownership of %c%d...\n",
              Shipltrs[ship->type], i);
      notify(Race->Playernum, 0, revoke_buf);
      putship(ship);
    }
    free(ship);
  }

  /*  And money too....  */

  sprintf(revoke_buf, "Transferring %ld money...\n", Race->governor[gov].money);
  notify(Race->Playernum, 0, revoke_buf);
  Race->governor[j].money = Race->governor[j].money + Race->governor[gov].money;
  Race->governor[gov].money = 0;

  /* And last but not least, flag the governor as inactive.... */

  Race->governor[gov].active = 0;
  strcpy(Race->governor[gov].password, "");
  strcpy(Race->governor[gov].name, "");
  sprintf(revoke_buf, "\n*** Governor [%d,%d]'s powers have been REVOKED ***\n",
          Race->Playernum, gov);
  notify(Race->Playernum, 0, revoke_buf);
  sprintf(revoke_buf, "rm %s.%d.%d", TELEGRAMFL, Race->Playernum, gov);
  system(revoke_buf); /*  Remove the telegram file too....  */

  return;
}

int authorized(int Governor, shiptype *ship) {
  return (!Governor || ship->governor == Governor);
}

int start_shiplist(int Playernum, int Governor, const char *p) {
  planettype *planet;
  shiptype *ship;
  int st, pl, sh;

  if (*p == '#')
    return (atoi(++p));
  if (isdigit(*p))
    return (atoi(p));

  /*ship number not given */
  st = Dir[Playernum - 1][Governor].snum;
  pl = Dir[Playernum - 1][Governor].pnum;
  switch (Dir[Playernum - 1][Governor].level) {
  case LEVEL_UNIV:
    getsdata(&Sdata);
    return Sdata.ships;
  case LEVEL_STAR:
    getstar(&Stars[st], st); /*Stars doesn't need to be freed */
    return Stars[st]->ships;
  case LEVEL_PLAN:
    getplanet(&planet, st, pl);
    sh = planet->ships;
    free(planet);
    return sh;
  case LEVEL_SHIP:
    (void)getship(&ship, Dir[Playernum - 1][Governor].shipno);
    sh = ship->ships;
    free(ship);
    return sh;
  }
  return 0;
}

/* Step through linked list at current player scope */
int do_shiplist(shiptype **s, int *nextshipno) {
  int shipno;
  if (!(shipno = *nextshipno))
    return 0;

  if (!getship(s, shipno)) /* allocate memory, free in loop */
    return 0;
  *nextshipno = (*s)->nextship;
  return shipno;
}

int in_list(int Playernum, char *list, shiptype *s, int *nextshipno) {
  char *p, q;
  if (s->owner != Playernum || !s->alive)
    return 0;
  q = Shipltrs[s->type];
  p = list;
  if (*p == '#' || isdigit(*p)) {
    if (s->owner != Playernum || !s->alive)
      return 0;
    *nextshipno = 0;
    return s->number;
  }
  for (; *p; p++)
    if (*p == q || *p == '*')
      return s->number; /* '*' is a wildcard */
  return 0;
}

/* Deity fix-it utilities */
void fix(int Playernum, int Governor) {
  planettype *p;
  shiptype *s;

  if (match(args[1], "planet")) {
    if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
      notify(Playernum, Governor, "Change scope to the planet first.\n");
      return;
    }
    getplanet(&p, Dir[Playernum - 1][Governor].snum,
              Dir[Playernum - 1][Governor].pnum);
    if (match(args[2], "Maxx")) {
      if (argn > 3)
        p->Maxx = atoi(args[3]);
      sprintf(buf, "Maxx = %d\n", p->Maxx);
    } else if (match(args[2], "Maxy")) {
      if (argn > 3)
        p->Maxy = atoi(args[3]);
      sprintf(buf, "Maxy = %d\n", p->Maxy);
    } else if (match(args[2], "xpos")) {
      if (argn > 3)
        p->xpos = (double)atoi(args[3]);
      sprintf(buf, "xpos = %f\n", p->xpos);
    } else if (match(args[2], "ypos")) {
      if (argn > 3)
        p->ypos = (double)atoi(args[3]);
      sprintf(buf, "ypos = %f\n", p->ypos);
    } else if (match(args[2], "ships")) {
      if (argn > 3)
        p->ships = atoi(args[3]);
      sprintf(buf, "ships = %d\n", p->ships);
    } else if (match(args[2], "sectormappos")) {
      if (argn > 3)
        p->sectormappos = atoi(args[3]);
      sprintf(buf, "sectormappos = %d\n", p->sectormappos);
    } else if (match(args[2], "rtemp")) {
      if (argn > 3)
        p->conditions[RTEMP] = atoi(args[3]);
      sprintf(buf, "RTEMP = %d\n", p->conditions[RTEMP]);
    } else if (match(args[2], "temperature")) {
      if (argn > 3)
        p->conditions[TEMP] = atoi(args[3]);
      sprintf(buf, "TEMP = %d\n", p->conditions[TEMP]);
    } else if (match(args[2], "methane")) {
      if (argn > 3)
        p->conditions[METHANE] = atoi(args[3]);
      sprintf(buf, "METHANE = %d\n", p->conditions[METHANE]);
    } else if (match(args[2], "oxygen")) {
      if (argn > 3)
        p->conditions[OXYGEN] = atoi(args[3]);
      sprintf(buf, "OXYGEN = %d\n", p->conditions[OXYGEN]);
    } else if (match(args[2], "co2")) {
      if (argn > 3)
        p->conditions[CO2] = atoi(args[3]);
      sprintf(buf, "CO2 = %d\n", p->conditions[CO2]);
    } else if (match(args[2], "hydrogen")) {
      if (argn > 3)
        p->conditions[HYDROGEN] = atoi(args[3]);
      sprintf(buf, "HYDROGEN = %d\n", p->conditions[HYDROGEN]);
    } else if (match(args[2], "nitrogen")) {
      if (argn > 3)
        p->conditions[NITROGEN] = atoi(args[3]);
      sprintf(buf, "NITROGEN = %d\n", p->conditions[NITROGEN]);
    } else if (match(args[2], "sulfur")) {
      if (argn > 3)
        p->conditions[SULFUR] = atoi(args[3]);
      sprintf(buf, "SULFUR = %d\n", p->conditions[SULFUR]);
    } else if (match(args[2], "helium")) {
      if (argn > 3)
        p->conditions[HELIUM] = atoi(args[3]);
      sprintf(buf, "HELIUM = %d\n", p->conditions[HELIUM]);
    } else if (match(args[2], "other")) {
      if (argn > 3)
        p->conditions[OTHER] = atoi(args[3]);
      sprintf(buf, "OTHER = %d\n", p->conditions[OTHER]);
    } else if (match(args[2], "toxic")) {
      if (argn > 3)
        p->conditions[TOXIC] = atoi(args[3]);
      sprintf(buf, "TOXIC = %d\n", p->conditions[TOXIC]);
    } else {
      notify(Playernum, Governor, "No such option for 'fix planet'.\n");
      free(p);
      return;
    }
    notify(Playernum, Governor, buf);
    if (argn > 3)
      putplanet(p, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
    free(p);
    return;
  }
  if (match(args[1], "ship")) {
    if (Dir[Playernum - 1][Governor].level != LEVEL_SHIP) {
      notify(Playernum, Governor,
             "Change scope to the ship you wish to fix.\n");
      return;
    }
    (void)getship(&s, Dir[Playernum - 1][Governor].shipno);
    if (match(args[2], "fuel")) {
      if (argn > 3)
        s->fuel = (double)atoi(args[3]);
      sprintf(buf, "fuel = %f\n", s->fuel);
    } else if (match(args[2], "max_fuel")) {
      if (argn > 3)
        s->max_fuel = atoi(args[3]);
      sprintf(buf, "fuel = %d\n", s->max_fuel);
    } else if (match(args[2], "destruct")) {
      if (argn > 3)
        s->destruct = atoi(args[3]);
      sprintf(buf, "destruct = %d\n", s->destruct);
    } else if (match(args[2], "resource")) {
      if (argn > 3)
        s->resource = atoi(args[3]);
      sprintf(buf, "resource = %d\n", s->resource);
    } else if (match(args[2], "damage")) {
      if (argn > 3)
        s->damage = atoi(args[3]);
      sprintf(buf, "damage = %d\n", s->damage);
    } else if (match(args[2], "alive")) {
      s->alive = 1;
      s->damage = 0;
      sprintf(buf, "%s resurrected\n", Ship(s));
    } else if (match(args[2], "dead")) {
      s->alive = 0;
      s->damage = 100;
      sprintf(buf, "%s destroyed\n", Ship(s));
    } else {
      notify(Playernum, Governor, "No such option for 'fix ship'.\n");
      free(s);
      return;
    }
    notify(Playernum, Governor, buf);
    putship(s);
    free(s);
    return;
  } else
    notify(Playernum, Governor, "Fix what?\n");
}

int match(const char *p, const char *q) { return (!strncmp(p, q, strlen(p))); }

void DontOwnErr(int Playernum, int Governor, int shipno) {
  sprintf(buf, "You don't own ship #%d.\n", shipno);
  notify(Playernum, Governor, buf);
}

int enufAP(int Playernum, int Governor, unsigned short AP, int x) {
  reg int blah;

  if ((blah = (AP < x))) {
    sprintf(buf, "You don't have %d action points there.\n", x);
    notify(Playernum, Governor, buf);
  }
  return (!blah);
}

void Getracenum(char *racepass, char *govpass, int *racenum, int *govnum) {
  reg int i, j;
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

int GetPlayer(char *name) {
  int rnum;
  reg int i;

  rnum = 0;

  if (isdigit(*name)) {
    if ((rnum = atoi(name)) < 1 || rnum > Num_races)
      return 0;
    return rnum;
  } else {
    for (i = 1; i <= Num_races; i++)
      if (match(name, races[i - 1]->name))
        return i;
    return 0;
  }
}

void allocateAPs(int Playernum, int Governor, int APcount) {
  int maxalloc;
  int alloc;

  if (Dir[Playernum - 1][Governor].level == LEVEL_UNIV) {
    sprintf(
        buf,
        "Change scope to the system you which to transfer global APs to.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  alloc = atoi(args[1]);
  if (alloc <= 0) {
    notify(Playernum, Governor,
           "You must specify a positive amount of APs to allocate.\n");
    return;
  }

  getsdata(&Sdata);
  maxalloc = MIN(
      Sdata.AP[Playernum - 1],
      LIMIT_APs - Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1]);
  if (alloc > maxalloc) {
    sprintf(buf, "Illegal value (%d) - maximum = %d\n", alloc, maxalloc);
    notify(Playernum, Governor, buf);
    return;
  }
  Sdata.AP[Playernum - 1] -= alloc;
  putsdata(&Sdata);
  getstar(&Stars[Dir[Playernum - 1][Governor].snum],
          Dir[Playernum - 1][Governor].snum);
  Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1] =
      MIN(LIMIT_APs,
          Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1] + alloc);
  putstar(Stars[Dir[Playernum - 1][Governor].snum],
          Dir[Playernum - 1][Governor].snum);
  sprintf(buf, "Allocated\n");
  notify(Playernum, Governor, buf);
}

void deductAPs(int Playernum, int Governor, int n, int snum, int sdata) {
  if (n) {

    if (!sdata) {
      getstar(&Stars[snum], snum);

      if (Stars[snum]->AP[Playernum - 1] >= n)
        Stars[snum]->AP[Playernum - 1] -= n;
      else {
        Stars[snum]->AP[Playernum - 1] = 0;
        sprintf(buf, "WHOA!  You cheater!  Oooohh!  OOOOH!\n  I'm "
                     "tellllllllliiiiiiinnnnnnnnnggggggggg!!!!!!!\n");
        notify(Playernum, Governor, buf);
      }

      putstar(Stars[snum], snum);

      if (Dir[Playernum - 1][Governor].level != LEVEL_UNIV &&
          Dir[Playernum - 1][Governor].snum == snum) {
        /* fix the prompt */
        sprintf(Dir[Playernum - 1][Governor].prompt + 5, "%02d",
                Stars[snum]->AP[Playernum - 1]);
        Dir[Playernum - 1][Governor].prompt[7] =
            ']'; /* fix bracket (made '\0' by sprintf)*/
      }
    } else {
      getsdata(&Sdata);
      Sdata.AP[Playernum - 1] = MAX(0, Sdata.AP[Playernum - 1] - n);
      putsdata(&Sdata);

      if (Dir[Playernum - 1][Governor].level == LEVEL_UNIV) {
        sprintf(Dir[Playernum - 1][Governor].prompt + 2, "%02d",
                Sdata.AP[Playernum - 1]);
        Dir[Playernum - 1][Governor].prompt[3] = ']';
      }
    }
  }
}

/* lists all ships in current scope for debugging purposes */
void list(int Playernum, int Governor) {
  shiptype *ship;
  planettype *p;
  int sh;

  switch (Dir[Playernum - 1][Governor].level) {
  case LEVEL_UNIV:
    sh = Sdata.ships;
    break;
  case LEVEL_STAR:
    getstar(&Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    sh = Stars[Dir[Playernum - 1][Governor].snum]->ships;
    break;
  case LEVEL_PLAN:
    getplanet(&p, Dir[Playernum - 1][Governor].snum,
              Dir[Playernum - 1][Governor].pnum);
    sh = p->ships;
    free(p);
    break;
  case LEVEL_SHIP:
    sh = Dir[Playernum - 1][Governor].shipno;
    break;
  }

  while (sh) {
    (void)getship(&ship, sh);
    sprintf(buf, "%15s #%d '%s' (pl %d) -> #%d %s\n", Shipnames[ship->type], sh,
            ship->name, ship->owner, ship->nextship,
            ship->alive ? "" : "(dead)");
    notify(Playernum, Governor, buf);
    sh = ship->nextship;
    free(ship);
  }
}

double morale_factor(double x) {
  return (atan((double)x / 10000.) / 3.14159565 + .5);
}

