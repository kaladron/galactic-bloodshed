// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TELE_H
#define TELE_H

void check_for_telegrams(GameObj &);
void purge();
void post(std::string, int);
void push_telegram_race(const player_t, const std::string &);
void teleg_read(GameObj &);
void news_read(NewsType type, GameObj &g);

#endif  // TELE_H
