/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * toxicty.c -- change threshold in toxicity to build a wc.
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
#include <math.h>
#include <signal.h>
#include <ctype.h>

void toxicity(int, int, int);
#include "GB_server.h"
#include "shlmisc.h"
#include "files_shl.h"

void toxicity(int Playernum, int Governor, int APcount) {
  int thresh;
  planettype *p;

  sscanf(args[1], "%d", &thresh);

  if (thresh > 100 || thresh < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  getplanet(&p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);
  p->info[Playernum - 1].tox_thresh = thresh;
  putplanet(p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);

  sprintf(buf, " New threshold is: %u\n", p->info[Playernum - 1].tox_thresh);
  notify(Playernum, Governor, buf);

  free(p);
}
