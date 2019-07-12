// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GETPLACE_H
#define GETPLACE_H

#include <string>

#include "gb/ships.h"

placetype getplace(GameObj &, const std::string &, const int);
char *Dispshiploc_brief(Ship *);
char *Dispshiploc(Ship *);
std::string Dispplace(const placetype &);
bool testship(const player_t, const governor_t, const Ship &);

#endif  // GETPLACE_H
