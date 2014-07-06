/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *  examine -- check out an object
 */

#include <string.h>
#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
extern long Shipdata[NUMSTYPES][NUMABILS];
extern char *Shipnames[];

void examine(int, int, int);
#include "files_shl.h"
#include "GB_server.h"
#include "shlmisc.h"

void examine(int Playernum, int Governor, int APcount) {
  shiptype *ship;
  int t, shipno;
  FILE *fd;
  char ch;

  if (argn < 2) {
    notify(Playernum, Governor, "Examine what?\n");
    return;
  }

  sscanf(args[1] + (*args[1] == '#'), "%d", &shipno);

  if (!getship(&ship, shipno)) {
    return;
  }
  if (!ship->alive) {
    sprintf(buf, "that ship is dead.\n");
    notify(Playernum, Governor, buf);
    free(ship);
    return;
  }
  if (ship->whatorbits == LEVEL_UNIV ||
      isclr(Stars[ship->storbits]->inhabited, Playernum)) {
    sprintf(buf, "That ship it not visible to you.\n");
    notify(Playernum, Governor, buf);
    free(ship);
    return;
  }

  if ((fd = fopen(EXAM_FL, "r")) == NULL) {
    perror(EXAM_FL);
    free(ship);
    return;
  }

  /* look through ship data file */
  for (t = 0; t <= ship->type; t++)
    while (fgetc(fd) != '~')
      ;

  /* look through ship data file */
  sprintf(buf, "\n");
  /* give report */
  while ((ch = fgetc(fd)) != '~') {
    sprintf(temp, "%c", ch);
    strcat(buf, temp);
  }
  notify(Playernum, Governor, buf);
  fclose(fd);

  if (!ship->examined) {
    if (ship->whatorbits == LEVEL_UNIV)
      deductAPs(Playernum, Governor, APcount, 0, 1); /* ded from sdata */
    else
      deductAPs(Playernum, Governor, APcount, (int)ship->storbits, 0);

    ship->examined = 1;
    putship(ship);
  }

  if (has_switch(ship)) {
    sprintf(buf,
            "This device has an on/off switch that can be set with order.\n");
    notify(Playernum, Governor, buf);
  }
  if (!ship->active) {
    sprintf(buf, "This device has been irradiated;\nit's crew is dying and it "
                 "cannot move for the time being.\n");
    notify(Playernum, Governor, buf);
  }
  free(ship);
}
