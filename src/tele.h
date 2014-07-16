// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TELE_H
#define TELE_H

void purge(void);
void post(const char *, int);
void push_telegram_race(int, char *);
void push_telegram(int, int, char *);
void teleg_read(int, int);
void news_read(int, int, int);

#endif // TELE_H
