// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHLMISC_H
#define SHLMISC_H

#include "races.h"
#include "ships.h"

char *Ship(shiptype *s);
void grant(int, int, int);
void governors(int, int, int);
void do_revoke(racetype *, int, int);
int authorized(int, shiptype *);
int start_shiplist(int, int, char *);
int do_shiplist(shiptype **, int *);
int in_list(int, char *, shiptype *, int *);
void fix(int, int);
int match(char *, char *);
void DontOwnErr(int, int, int);
int enufAP(int, int, unsigned short, int);
void Getracenum(char *, char *, int *, int *);
int GetPlayer(char *);
void allocateAPs(int, int, int);
void deductAPs(int, int, int, int, int);
void list(int, int);
double morale_factor(double);

#endif // SHLMISC_H
