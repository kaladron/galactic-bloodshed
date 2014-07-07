// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// Collection of globals that must be initialized for every standalone
// program.

#ifndef GLOBALS_H
#define GLOBALS_H

#include "doturn.h"
#include "power.h"
#include "races.h"
#include "ships.h"
#include "vars.h"

struct power Power[MAXPLAYERS];
struct block Blocks[MAXPLAYERS];
struct power_blocks Power_blocks;

racetype *races[MAXPLAYERS];

char buf[2047];
char long_buf[1024], short_buf[256];
char telegram_buf[AUTO_TELEG_SIZE];
char temp[128];

int ShipVector[NUMSTYPES];
shiptype **ships;

/* defense 5 is inpenetrable */
const int Defensedata[] = { 1, 1, 3, 2, 2, 3, 2, 4, 0 };

#ifdef MARKET
const char *Commod[] = { "resources", "destruct", "fuel", "crystals" };
#endif

struct stinfo Stinfo[NUMSTARS][MAXPLANETS];
struct vnbrain VN_brain;
struct sectinfo Sectinfo[MAX_X][MAX_Y];

const char Psymbol[] = { '@', 'o', 'O', '#', '~', '.', ')', '-' };
const char *Planet_types[] = { "Class M", "Asteroid",  "Airless", "Iceball",
                               "Jovian",  "Waterball", "Forest",  "Desert" };

long next_update_time;  /* When will next update be... approximately */
long next_segment_time; /* When will next segment be... approximately */
int update_time; /* Interval between updates */
int nsegments_done;    /* How many movements have we done so far? */

#endif // GLOBALS_H
