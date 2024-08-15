// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// Collection of globals that must be initialized for every standalone
// program.

#ifndef GLOBALS_H
#define GLOBALS_H

#include "gb/doturn.h"
#include "gb/map.h"

char buf[2047];
char long_buf[1024], short_buf[256];
char telegram_buf[AUTO_TELEG_SIZE];

#ifdef MARKET
const char *commod_name[] = {"resources", "destruct", "fuel", "crystals"};
#endif

struct stinfo Stinfo[NUMSTARS][MAXPLANETS];
struct vnbrain VN_brain;
struct sectinfo Sectinfo[MAX_X][MAX_Y];

const char Psymbol[] = {'@', 'o', 'O', '#', '~', '.', ')', '-'};
const char *Planet_types[] = {"Class M", "Asteroid",  "Airless", "Iceball",
                              "Jovian",  "Waterball", "Forest",  "Desert"};

time_t next_update_time;   /* When will next update be... approximately */
time_t next_segment_time;  /* When will next segment be... approximately */
unsigned int update_time;  /* Interval between updates in minutes */
segments_t nsegments_done; /* How many movements have we done so far? */

const char *Desnames[] = {"ocean",  "land",   "mountainous", "gaseous", "ice",
                          "forest", "desert", "plated",      "wasted"};

const char Dessymbols[] = {CHAR_SEA,    CHAR_LAND,   CHAR_MOUNT,
                           CHAR_GAS,    CHAR_ICE,    CHAR_FOREST,
                           CHAR_DESERT, CHAR_PLATED, CHAR_WASTED};

#endif  // GLOBALS_H
