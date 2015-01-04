// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FILES_SHL_H
#define FILES_SHL_H

#include <memory>
#include <stdint.h>

#include "races.h"
#include "ships.h"
#include "vars.h"

void opensql(void);

void close_file(int);
void open_data_files(void);
void close_data_files(void);
void openstardata(int *);
void openshdata(int *);
void opencommoddata(int *);
void openpdata(int *);
void opensectdata(int *);
void openracedata(int *);
void getsdata(struct stardata *S);
void getrace(racetype **, int);
void getstar(startype **, int);
void getplanet(planettype **, starnum_t, planetnum_t);
int getship(shiptype **, shipnum_t);
int getcommod(commodtype **, commodnum_t);
sector getsector(const planettype &, const int x, const int y);
sector_map getsmap(const planettype &);
int getdeadship(void);
int getdeadcommod(void);
void initsqldata(void);
void putsdata(struct stardata *);
void putrace(racetype *);
void putstar(startype *, starnum_t);
void putplanet(planettype *, startype *, int);
void putplanet_sql(planettype *, startype *, int);
void putsector(const sector &, const planettype &, const int x, const int y);
void putsmap(sector_map &map, planettype &p);
void putship(shiptype *);
void putcommod(commodtype *, int);
int Numraces(void);
shipnum_t Numships(void);
int Numcommods(void);
int Newslength(int);
void clr_shipfree(void);
void clr_commodfree(void);
void makeshipdead(int);
void makecommoddead(int);
void Putpower(struct power[MAXPLAYERS]);
void Getpower(struct power[MAXPLAYERS]);
void Putblock(struct block[MAXPLAYERS]);
void Getblock(struct block[MAXPLAYERS]);

#endif  // FILES_SHL_H
