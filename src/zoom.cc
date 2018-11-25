// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* zoom.c -- zoom in or out for orbit display */

#include "zoom.h"

#include <stdio.h>

#include "GB_server.h"
#include "buffers.h"
#include "vars.h"

void zoom(const command_t &argv, const player_t Playernum,
          const governor_t Governor) {
  int i = (Dir[Playernum - 1][Governor].level == LEVEL_UNIV);

  if (argn > 1) {
    double num, denom;
    if (sscanf(argv[1].c_str(), "%lf/%lf", &num, &denom) == 2) {
      /* num/denom format */
      if (denom == 0.0) {
        sprintf(buf, "Illegal denominator value.\n");
        notify(Playernum, Governor, buf);
      } else
        Dir[Playernum - 1][Governor].zoom[i] = num / denom;
    } else {
      /* one number */
      Dir[Playernum - 1][Governor].zoom[i] = num;
    }
  }

  sprintf(buf, "Zoom value %g, lastx = %g, lasty = %g.\n",
          Dir[Playernum - 1][Governor].zoom[i],
          Dir[Playernum - 1][Governor].lastx[i],
          Dir[Playernum - 1][Governor].lasty[i]);
  notify(Playernum, Governor, buf);
}
