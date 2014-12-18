// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DECLARE_H
#define DECLARE_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void invite(const command_t &argv, const player_t Playernum,
            const governor_t Governor);
void declare(const command_t &argv, const player_t Playernum,
             const governor_t Governor);
void vote(const command_t &argv, const player_t Playernum,
          const governor_t Governor);
void pledge(const command_t &argv, const player_t Playernum,
            const governor_t Governor);

#endif  // DECLARE_H
