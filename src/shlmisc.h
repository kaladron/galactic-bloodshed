// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHLMISC_H
#define SHLMISC_H

#include <string>

#include "races.h"
#include "ships.h"
#include "vars.h"

std::string Ship(const ship &);
void grant(const command_t &, GameObj &);
void governors(const command_t &, GameObj &);
int authorized(int, shiptype *);
int start_shiplist(GameObj &, const char *);
shipnum_t do_shiplist(shiptype **, shipnum_t *);
int in_list(player_t, const char *, shiptype *, shipnum_t *);
void fix(const command_t &, GameObj &);
void DontOwnErr(int, int, shipnum_t);
int enufAP(int, int, unsigned short, int);
void Getracenum(char *, char *, int *, int *);
player_t get_player(const std::string &);
void allocateAPs(const command_t &, GameObj &);
void deductAPs(const player_t, const governor_t, unsigned int, starnum_t, int);
double morale_factor(double);

#endif  // SHLMISC_H
