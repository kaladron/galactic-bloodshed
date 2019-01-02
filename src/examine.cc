// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* examine -- check out an object */

#include "examine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "GB_server.h"
#include "buffers.h"
#include "files.h"
#include "files_shl.h"
#include "ships.h"
#include "shlmisc.h"
#include "vars.h"

void examine(const command_t &argv, GameObj &g) {
  const int APcount = 0;
  FILE *fd;
  char ch;

  if (argv.size() < 2) {
    notify(g.player, g.governor, "Examine what?\n");
    return;
  }

  auto shipno = string_to_shipnum(argv[1]);

  if (!shipno) {
    return;
  }

  auto ship = getship(*shipno);

  if (!ship) {
    return;
  }

  if (!ship->alive) {
    g.out << "that ship is dead.\n";
    return;
  }
  if (ship->whatorbits == ScopeLevel::LEVEL_UNIV ||
      isclr(Stars[ship->storbits]->inhabited, g.player)) {
    g.out << "That ship it not visible to you.\n";
    return;
  }

  if ((fd = fopen(EXAM_FL, "r")) == nullptr) {
    perror(EXAM_FL);
    return;
  }

  /* look through ship data file */
  for (int t = 0; t <= ship->type; t++)
    while (fgetc(fd) != '~')
      ;

  /* look through ship data file */
  sprintf(buf, "\n");
  /* give report */
  while ((ch = fgetc(fd)) != '~') {
    sprintf(temp, "%c", ch);
    strcat(buf, temp);
  }
  notify(g.player, g.governor, buf);
  fclose(fd);

  if (!ship->examined) {
    if (ship->whatorbits == ScopeLevel::LEVEL_UNIV)
      deductAPs(g.player, g.governor, APcount, 0, 1); /* ded from sdata */
    else
      deductAPs(g.player, g.governor, APcount, (int)ship->storbits, 0);

    ship->examined = 1;
    putship(&*ship);
  }

  if (has_switch(&*ship)) {
    g.out << "This device has an on/off switch that can be set with order.\n";
  }
  if (!ship->active) {
    g.out << "This device has been irradiated;\nit's crew is dying and it "
             "cannot move for the time being.\n";
  }
}
