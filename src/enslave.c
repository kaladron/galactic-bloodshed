// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* enslave.c -- ENSLAVE the planet below. */

#include "enslave.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "getplace.h"
#include "max.h"
#include "races.h"
#include "ships.h"
#include "shlmisc.h"
#include "vars.h"

void enslave(int Playernum, int Governor, int APcount) {
  shiptype *s, *s2;
  planettype *p;
  int sh, shipno, i, aliens = 0, def = 0, attack = 0;
  racetype *Race;

  sscanf(args[1] + (args[1][0] == '#'), "%d", &shipno);

  if (!getship(&s, shipno)) {
    return;
  }
  if (testship(Playernum, Governor, s)) {
    free(s);
    return;
  }
  if (s->type != STYPE_OAP) {
    sprintf(buf, "This ship is not an %s.\n", Shipnames[STYPE_OAP]);
    notify(Playernum, Governor, buf);
    free(s);
    return;
  }
  if (s->whatorbits != LEVEL_PLAN) {
    sprintf(buf, "%s doesn't orbit a planet.\n", Ship(s));
    notify(Playernum, Governor, buf);
    free(s);
    return;
  }
  if (!enufAP(Playernum, Governor, Stars[s->storbits]->AP[Playernum - 1],
              APcount)) {
    free(s);
    return;
  }
  getplanet(&p, (int)s->storbits, (int)s->pnumorbits);
  if (p->info[Playernum - 1].numsectsowned == 0) {
    sprintf(buf, "You don't have a garrison on the planet.\n");
    notify(Playernum, Governor, buf);
    free(s);
    free(p);
    return;
  }

  /* add up forces attacking, defending */
  for (attack = aliens = def = 0, i = 1; i < MAXPLAYERS; i++) {
    if (p->info[i - 1].numsectsowned && i != Playernum) {
      aliens = 1;
      def += p->info[i - 1].destruct;
    }
  }

  if (!aliens) {
    sprintf(buf, "There is no one else on this planet to enslave!\n");
    notify(Playernum, Governor, buf);
    free(s);
    free(p);
    return;
  }

  Race = races[Playernum - 1];

  sh = p->ships;
  while (sh) {
    (void)getship(&s2, sh);
    if (s2->alive && s2->active) {
      if (p->info[s2->owner].numsectsowned && s2->owner != Playernum)
        def += s2->destruct;
      else if (s2->owner == Playernum)
        attack += s2->destruct;
    }
    sh = s2->nextship;
    free(s2);
  }

  deductAPs(Playernum, Governor, APcount, (int)s->storbits, 0);

  sprintf(buf,
          "\nFor successful enslavement this ship and the other ships here\n");
  notify(Playernum, Governor, buf);
  sprintf(buf, "that are yours must have a weapons\n");
  notify(Playernum, Governor, buf);
  sprintf(buf,
          "capacity greater than twice that the enemy can muster, including\n");
  notify(Playernum, Governor, buf);
  sprintf(buf, "the planet and all ships orbiting it.\n");
  notify(Playernum, Governor, buf);
  sprintf(buf, "\nTotal forces bearing on %s:   %d\n", prin_ship_orbits(s),
          attack);
  notify(Playernum, Governor, buf);

  sprintf(telegram_buf, "ALERT!!!\n\nPlanet /%s/%s ", Stars[s->storbits]->name,
          Stars[s->storbits]->pnames[s->pnumorbits]);

  if (def <= 2 * attack) {
    p->slaved_to = Playernum;
    putplanet(p, (int)s->storbits, (int)s->pnumorbits);

    /* send telegs to anyone there */
    sprintf(buf, "ENSLAVED by %s!!\n", Ship(s));
    strcat(telegram_buf, buf);
    sprintf(buf, "All material produced here will be\ndiverted to %s coffers.",
            Race->name);
    strcat(telegram_buf, buf);

    sprintf(buf,
            "\nEnslavement successful.  All material produced here will\n");
    notify(Playernum, Governor, buf);
    sprintf(buf, "be diverted to %s.\n", Race->name);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "You must maintain a garrison of 0.1%% the population of the\n");
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "planet (at least %.0f); otherwise there is a 50%% chance that\n",
            p->popn * 0.001);
    notify(Playernum, Governor, buf);
    sprintf(buf, "enslaved population will revolt.\n");
    notify(Playernum, Governor, buf);
  } else {
    sprintf(buf, "repulsed attempt at enslavement by %s!!\n", Ship(s));
    strcat(telegram_buf, buf);
    sprintf(buf, "Enslavement repulsed, defense/attack Ratio : %d to %d.\n",
            def, attack);
    strcat(telegram_buf, buf);

    sprintf(buf, "Enslavement repulsed.\n");
    notify(Playernum, Governor, buf);
    sprintf(buf, "You needed more weapons bearing on the planet...\n");
    notify(Playernum, Governor, buf);
  }

  for (i = 1; i < MAXPLAYERS; i++)
    if (p->info[i - 1].numsectsowned && i != Playernum)
      warn(i, (int)Stars[s->storbits]->governor[i - 1], telegram_buf);

  free(p);
  free(s);
}
