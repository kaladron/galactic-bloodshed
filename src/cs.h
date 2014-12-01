// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef CS_H
#define CS_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void center(const command_t &argv, const player_t Playernum,
            const governor_t Governor);
void do_prompt(int, int);
void cs(const command_t &argv, const player_t Playernum,
        const governor_t Governor);

#endif  // CS_H
