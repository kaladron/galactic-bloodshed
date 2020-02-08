// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHLMISC_H
#define SHLMISC_H

#include "gb/races.h"
#include "gb/ships.h"
#include "gb/vars.h"

bool authorized(governor_t, const Ship &);
shipnum_t start_shiplist(GameObj &, const std::string_view);
shipnum_t do_shiplist(Ship **, shipnum_t *);
bool in_list(const player_t, std::string_view, const Ship &, shipnum_t *);
void DontOwnErr(int, int, shipnum_t);
bool enufAP(player_t, governor_t, ap_t have, ap_t needed);
std::tuple<player_t, governor_t> getracenum(const std::string &,
                                            const std::string &);
void get4args(const char *s, int *xl, int *xh, int *yl, int *yh);
player_t get_player(const std::string &);
void allocateAPs(const command_t &, GameObj &);
void deductAPs(const GameObj &, unsigned int, ScopeLevel);
void deductAPs(const GameObj &, unsigned int, starnum_t);

#endif  // SHLMISC_H
