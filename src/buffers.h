// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef BUFFERS_H
#define BUFFERS_H

#include "tweakables.h"

EXTERN char buf[2047];
EXTERN char long_buf[1024], short_buf[256];
EXTERN char telegram_buf[AUTO_TELEG_SIZE];
EXTERN char temp[128];

#endif // BUFFERS_H
