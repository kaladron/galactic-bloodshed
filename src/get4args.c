// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "get4args.h"

#include <stdio.h>

void get4args(char *s, int *xl, int *xh, int *yl, int *yh) {
  char *p, s1[17], s2[17];
  p = s;

  sscanf(p, "%[^,]", s1);
  while ((*p != ':') && (*p != ','))
    p++;
  if (*p == ':') {
    sscanf(s1, "%d:%d", xl, xh);
    while (*p != ',')
      p++;
  } else if (*p == ',') {
    sscanf(s1, "%d", xl);
    *xh = (*xl);
  }

  sscanf(p, "%s", s2);
  while ((*p != ':') && (*p != '\0'))
    p++;
  if (*p == ':') {
    sscanf(s2, ",%d:%d", yl, yh);
  } else {
    sscanf(s2, ",%d,", yl);
    *yh = (*yl);
  }
}
