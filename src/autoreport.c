/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.c.
 * Restrictions in GB_copyright.h.c.
 * autoreport.c -- tell server to generate a report for each planet
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

void autoreport(int, int, int);
#include "getplace.h"
#include "GB_server.h"
#include "files_shl.h"

void autoreport(int Playernum, int Governor, int APcount) {
  planettype *p;
  placetype place;
  int snum, pnum;

  snum = Dir[Playernum - 1][Governor].snum;
  pnum = Dir[Playernum - 1][Governor].pnum;

  if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
    notify(Playernum, Governor, "You are not authorized to do this here.\n");
    return;
  }

  if (argn == 1) { /* no args */
    if (Dir[Playernum - 1][Governor].level == LEVEL_PLAN) {
      getplanet(&p, snum, pnum);
      if (p->info[Playernum - 1].autorep)
        p->info[Playernum - 1].autorep = 0;
      else
        p->info[Playernum - 1].autorep = TELEG_MAX_AUTO;
      putplanet(p, snum, pnum);

      sprintf(buf, "Autoreport on %s has been %s.\n", Stars[snum]->pnames[pnum],
              p->info[Playernum - 1].autorep ? "set" : "unset");
      notify(Playernum, Governor, buf);
      free(p);
    } else {
      sprintf(buf, "Scope must be a planet.\n");
      notify(Playernum, Governor, buf);
    }
  } else if (argn > 1) { /* argn==2, place specified */
    place = Getplace(Playernum, Governor, args[1], 0);
    if (place.level == LEVEL_PLAN) {
      getplanet(&p, snum, pnum);
      sprintf(buf, "Autoreport on %s has been %s.\n", Stars[snum]->pnames[pnum],
              p->info[Playernum - 1].autorep ? "set" : "unset");
      notify(Playernum, Governor, buf);
      p->info[Playernum - 1].autorep = !p->info[Playernum - 1].autorep;
      putplanet(p, snum, pnum);
      free(p);
    } else {
      sprintf(buf, "Scope must be a planet.\n");
      notify(Playernum, Governor, buf);
    }
  }
}
