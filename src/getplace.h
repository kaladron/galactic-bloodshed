// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GETPLACE_H
#define GETPLACE_H

#include "ships.h"

extern placetype Getplace(int, int, char *, int);
extern placetype Getplace2(int, int, char *, placetype *, int, int);
extern char *Dispshiploc_brief(shiptype *);
extern char *Dispshiploc(shiptype *);
extern char *Dispplace(int, int, placetype *);
extern int testship(int, int, shiptype *);

#endif // GETPLACE_H
