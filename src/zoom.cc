// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* zoom.c -- zoom in or out for orbit display */

#include "zoom.h"

#include <stdio.h>

#include "GB_server.h"
#include "buffers.h"
#include "vars.h"

void zoom(int Playernum, int Governor, int APcount) {
  double num, denom;
  int i;

  i = (Dir[Playernum - 1][Governor].level == LEVEL_UNIV);

  if (argn > 1) {
    if (sscanf(args[1], "%lf/%lf", &num, &denom) == 2) {
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
