// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOBILIZ_H
#define MOBILIZ_H

#include "vars.h"

void mobilize(const command_t &, GameObj &);
void tax(const command_t &, GameObj &);
int control(int, int, startype *);

#endif  // MOBILIZ_H
