// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// Collection of globals that must be initialized for every standalone
// program.

#ifndef GLOBALS_H
#define GLOBALS_H

#include "gb/tweakables.h"

char buf[2047];
char long_buf[1024], short_buf[256];
char telegram_buf[AUTO_TELEG_SIZE];

#endif  // GLOBALS_H
