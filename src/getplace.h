// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GETPLACE_H
#define GETPLACE_H

#include "ships.h"

placetype Getplace(int, int, char *, int);
char *Dispshiploc_brief(shiptype *);
char *Dispshiploc(shiptype *);
char *Dispplace(int, int, placetype *);
int testship(int, int, shiptype *);

#endif // GETPLACE_H
