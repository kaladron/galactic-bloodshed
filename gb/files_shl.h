// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FILES_SHL_H
#define FILES_SHL_H

#include <cstdint>
#include <memory>
#include <optional>

#include "gb/races.h"
#include "gb/ships.h"
#include "gb/vars.h"

class Db {};

class Sql : public Db {
 public:
  Sql();
};

void close_file(int);
Db open_data_files();
void close_data_files();
void openstardata(int *);
void openshdata(int *);
void opencommoddata(int *);
void opensectdata(int *);
void openracedata(int *);
void getsdata(struct stardata *S);
void getrace(Race **, int);
void getstar(startype **, int);
Planet getplanet(const starnum_t, const planetnum_t);
std::optional<Ship> getship(const shipnum_t);
std::optional<Ship> getship(Ship **, const shipnum_t);
int getcommod(commodtype **, commodnum_t);
Sector getsector(const Planet &, const int x, const int y);
SectorMap getsmap(const Planet &);
int getdeadship();
int getdeadcommod();
void initsqldata();
void putsdata(struct stardata *);
void putrace(Race *);
void putstar(startype *, starnum_t);
void putplanet(const Planet &, startype *, const int);
void putsector(const Sector &, const Planet &, const int x, const int y);
void putsmap(SectorMap &map, Planet &p);
void putship(Ship *);
void putcommod(commodtype *, int);
int Numraces();
shipnum_t Numships();
int Numcommods();
int Newslength(int);
void clr_shipfree();
void clr_commodfree();
void makeshipdead(int);
void makecommoddead(int);
void Putpower(struct power[MAXPLAYERS]);
void Getpower(struct power[MAXPLAYERS]);
void Putblock(struct block[MAXPLAYERS]);
void Getblock(struct block[MAXPLAYERS]);

#endif  // FILES_SHL_H
