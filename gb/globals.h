// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// Collection of globals that must be initialized for every standalone
// program.

#ifndef GLOBALS_H
#define GLOBALS_H

#include "gb/doturn.h"
#include "gb/map.h"
#include "gb/power.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/vars.h"

struct power Power[MAXPLAYERS];
struct block Blocks[MAXPLAYERS];
struct power_blocks Power_blocks;

char buf[2047];
char long_buf[1024], short_buf[256];
char telegram_buf[AUTO_TELEG_SIZE];
char temp[2047];

int ShipVector[NUMSTYPES];
Ship **ships;

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

struct stardata Sdata;

unsigned char Nuked[MAXPLAYERS];
unsigned long StarsInhab[NUMSTARS];
unsigned long StarsExpl[NUMSTARS];
std::vector<Star> stars;
unsigned short Sdatanumships[MAXPLAYERS];
unsigned long Sdatapopns[MAXPLAYERS];
unsigned short starnumships[NUMSTARS][MAXPLAYERS];
unsigned long starpopns[NUMSTARS][MAXPLAYERS];

unsigned long tot_resdep, prod_eff, prod_res[MAXPLAYERS];
unsigned long prod_fuel[MAXPLAYERS], prod_destruct[MAXPLAYERS];
unsigned long prod_crystals[MAXPLAYERS], prod_money[MAXPLAYERS];
unsigned long tot_captured, prod_mob;
unsigned long avg_mob[MAXPLAYERS];
unsigned char sects_gained[MAXPLAYERS], sects_lost[MAXPLAYERS];
unsigned char Claims;

std::array<std::array<std::unique_ptr<Planet>, MAXPLANETS>, NUMSTARS> planets;
unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];
uint64_t inhabited[NUMSTARS];
double Compat[MAXPLAYERS];
player_t Num_races;
unsigned long Num_commods;
shipnum_t Num_ships;
planetnum_t Planet_count;
unsigned long newslength[4];

unsigned long segments;

#endif  // GLOBALS_H
