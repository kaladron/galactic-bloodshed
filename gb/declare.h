// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DECLARE_H
#define DECLARE_H

#include "gb/vars.h"

void invite(const command_t &, GameObj &);
void declare(const command_t &, GameObj &);
void vote(const command_t &, GameObj &);
void pledge(const command_t &, GameObj &);

#endif  // DECLARE_H
