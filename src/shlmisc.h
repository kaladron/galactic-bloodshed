// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHLMISC_H
#define SHLMISC_H

#include <string>

#include "races.h"
#include "ships.h"

std::string Ship(const ship &);
void grant(int, int, int);
void governors(int, int, int);
int authorized(int, shiptype *);
int start_shiplist(int, int, const char *);
shipnum_t do_shiplist(shiptype **, shipnum_t *);
int in_list(player_t, const char *, shiptype *, shipnum_t *);
void fix(int, int);
int match(const char *, const char *);
void DontOwnErr(int, int, shipnum_t);
int enufAP(int, int, unsigned short, int);
void Getracenum(char *, char *, int *, int *);
player_t GetPlayer(const char *);
player_t GetPlayer(const std::string &);
void allocateAPs(int, int, int);
void deductAPs(int, int, int, int, int);
double morale_factor(double);

#endif  // SHLMISC_H
