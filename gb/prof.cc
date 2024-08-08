// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* prof.c -- print out racial profile */

import gblib;
import std.compat;

#include "gb/prof.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/races.h"
#include "gb/shootblast.h"
#include "gb/tweakables.h"

static int round_perc(int, const Race &, int);

static char est_buf[20];

char *Estimate_f(double data, const Race &r, int p) {
  int est;

  sprintf(est_buf, "?");

  if (r.translate[p - 1] > 10) {
    est = round_perc((int)data, r, p);
    if (est < 1000)
      sprintf(est_buf, "%d", est);
    else if (est < 10000)
      sprintf(est_buf, "%.1fK", (double)est / 1000.);
    else if (est < 1000000)
      sprintf(est_buf, "%.0fK", (double)est / 1000.);
    else
      sprintf(est_buf, "%.1fM", (double)est / 1000000.);
  }
  return est_buf;
}

char *Estimate_i(int data, const Race &r, unsigned int p) {
  int est;

  sprintf(est_buf, "?");

  if (r.translate[p - 1] > 10) {
    est = round_perc((int)data, r, p);
    if ((int)abs(est) < 1000)
      sprintf(est_buf, "%d", est);
    else if ((int)abs(est) < 10000)
      sprintf(est_buf, "%.1fK", (double)est / 1000.);
    else if ((int)abs(est) < 1000000)
      sprintf(est_buf, "%.0fK", (double)est / 1000.);
    else
      sprintf(est_buf, "%.1fM", (double)est / 1000000.);
  }
  return est_buf;
}

static int round_perc(const int data, const Race &r, int p) {
  int k = 101 - MIN(r.translate[p - 1], 100);
  return ((data / k) * k);
}
