// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAKESTAR_H
#define MAKESTAR_H

int Temperature(double dist, int stemp);
void Makestar_init();
Star Makestar(Database& db, starnum_t);
void Makeplanet_init();
void PrintStatistics();

#endif  // MAKESTAR_H
