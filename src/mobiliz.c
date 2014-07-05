/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * mobiliz.c -- persuade people to build military stuff.
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "buffers.h"
#include "races.h"
#include "power.h"
#include <signal.h>
#include <ctype.h>

void mobilize(int, int, int);
void tax(int, int, int);
int control(int, int, startype *);
#include "GB_server.p"
#include "shlmisc.p"
#include "files_shl.p"

void mobilize(int Playernum, int Governor, int APcount) {
  int sum_mob = 0;
  planettype *p;

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do this here.\n");
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  getplanet(&p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  getsmap(Smap, p);

  if (argn < 2) {
    sprintf(buf, "Current mobilization: %d    Quota: %d\n",
            p->info[Playernum - 1].comread, p->info[Playernum - 1].mob_set);
    notify(Playernum, Governor, buf);
    free(p);
    return;
  }
  sum_mob = atoi(args[1]);

  if (sum_mob > 100 || sum_mob < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    free(p);
    return;
  }
  p->info[Playernum - 1].mob_set = sum_mob;
  putplanet(p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);

  free(p);
}

void tax(int Playernum, int Governor, int APcount) {
  int sum_tax = 0;
  planettype *p;
  racetype *Race;

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  Race = races[Playernum - 1];
  if (!Race->Gov_ship) {
    notify(Playernum, Governor, "You have no government center active.\n");
    return;
  }
  if (Race->Guest) {
    notify(Playernum, Governor,
           "Sorry, but you can't do this when you are a guest.\n");
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  getplanet(&p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  if (argn < 2) {
    sprintf(buf, "Current tax rate: %d%%    Target: %d%%\n",
            p->info[Playernum - 1].tax, p->info[Playernum - 1].newtax);
    notify(Playernum, Governor, buf);
    free(p);
    return;
  }

  sum_tax = atoi(args[1]);

  if (sum_tax > 100 || sum_tax < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    free(p);
    return;
  }
  p->info[Playernum - 1].newtax = sum_tax;
  putplanet(p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);
  notify(Playernum, Governor, "Set.\n");
  free(p);
}

int control(int Playernum, int Governor, startype *star) {
  return (!Governor || star->governor[Playernum - 1] == Governor);
}
