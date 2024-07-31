// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FILES_SHL_H
#define FILES_SHL_H

#include "gb/races.h"
#include "gb/ships.h"
#include "gb/sql/dbdecl.h"
#include "gb/vars.h"

void close_file(int);
void openstardata(int *);
void openshdata(int *);
void opencommoddata(int *);
void opensectdata(int *);
void openracedata(int *);
void getsdata(stardata *S);
Race getrace(player_t);
Star getstar(starnum_t);
Planet getplanet(const starnum_t, const planetnum_t);
std::optional<Ship> getship(std::string_view shipstring);
std::optional<Ship> getship(const shipnum_t);
std::optional<Ship> getship(Ship **, const shipnum_t);
Commod getcommod(commodnum_t);
Sector getsector(const Planet &, const int x, const int y);
SectorMap getsmap(const Planet &);
int getdeadship();
int getdeadcommod();
void initsqldata();
void putsdata(stardata *);
void putrace(const Race &);
void putstar(const Star &, starnum_t);
void putplanet(const Planet &, const Star &, const planetnum_t);
void putsector(const Sector &, const Planet &);
void putsector(const Sector &, const Planet &, const int x, const int y);
void putsmap(SectorMap &map, Planet &p);
void putship(Ship *);
void putcommod(const Commod &, int);
shipnum_t Numships();
int Newslength(int);
void clr_shipfree();
void clr_commodfree();
void makeshipdead(int);
void makecommoddead(int);
void putpower(struct power[MAXPLAYERS]);
void getpower(struct power[MAXPLAYERS]);
void Putblock(struct block[MAXPLAYERS]);
void Getblock(struct block[MAXPLAYERS]);
void open_files();
void close_files();

#endif  // FILES_SHL_H
