// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GETPLACE_H
#define GETPLACE_H

#include <string>

#include "ships.h"

placetype Getplace(GameObj &, const std::string &, const int);
char *Dispshiploc_brief(Ship *);
char *Dispshiploc(Ship *);
std::string Dispplace(const placetype &);
int testship(int, int, Ship *);

#endif  // GETPLACE_H
