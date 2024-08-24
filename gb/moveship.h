// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVESHIP_H
#define MOVESHIP_H

void moveship(Ship &ship, int x, int y, int z);
void msg_OOF(const Ship &ship);
bool followable(const Ship &ship, Ship &target);

#endif  // MOVESHIP_H
