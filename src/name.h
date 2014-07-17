// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef NAME_H
#define NAME_H

#include "vars.h"

void personal(int, int, const char *);
void bless(int, int, int);
void insurgency(int, int, int);
void pay(int, int, int);
void give(int, int, int);
void page(int, int, int);
void send_message(int, int, int, int);
void read_messages(int, int, int);
void motto(int, int, int, const char *);
void name(int, int, int);
int MostAPs(int, startype *);
void announce(int, int, const char *, int);

#endif // NAME_H
