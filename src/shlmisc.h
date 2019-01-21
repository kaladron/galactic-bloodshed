// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHLMISC_H
#define SHLMISC_H

#include <optional>
#include <string>

#include "races.h"
#include "ships.h"
#include "vars.h"

std::optional<shipnum_t> string_to_shipnum(std::string_view);
std::string ship_to_string(const Ship &);
void grant(const command_t &, GameObj &);
void governors(const command_t &, GameObj &);
int authorized(int, Ship *);
shipnum_t start_shiplist(GameObj &, const std::string_view);
shipnum_t do_shiplist(Ship **, shipnum_t *);
bool in_list(player_t, const char *, Ship *, shipnum_t *);
void fix(const command_t &, GameObj &);
void DontOwnErr(int, int, shipnum_t);
int enufAP(int, int, unsigned short, int);
std::tuple<player_t, governor_t> getracenum(const std::string &,
                                            const std::string &);
player_t get_player(const std::string &);
void allocateAPs(const command_t &, GameObj &);
void deductAPs(const player_t, const governor_t, unsigned int, starnum_t, int);
double morale_factor(double);

#endif  // SHLMISC_H
