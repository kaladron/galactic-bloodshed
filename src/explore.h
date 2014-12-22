// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef EXPLORE_H
#define EXPLORE_H

#include "races.h"

#include "races.h"
#include "ships.h"
#include "vars.h"

void colonies(const command_t &argv, const player_t Playernum,
              const governor_t Governor);
void distance(const command_t &argv, const player_t Playernum,
              const governor_t Governor);
void exploration(const command_t &argv, const player_t Playernum,
                 const governor_t Governor);
void star_locations(const command_t &argv, const player_t Playernum,
                    const governor_t Governor);
void tech_status(const command_t &argv, const player_t Playernum,
                 const governor_t Governor);

#endif  // EXPLORE_H
