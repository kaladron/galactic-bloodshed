// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* tech.c -- increase investment in technological development. */

#include "gb/tech.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

double tech_prod(int investment, int popn) {
  double scale = (double)popn / 10000.;
  return (TECH_INVEST * log10((double)investment * scale + 1.0));
}
