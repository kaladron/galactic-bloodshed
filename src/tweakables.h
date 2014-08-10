// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 *  tweakable constants & other things -- changing the following may cause GB
 *	to freak if
 *	the functions using them are not recompiled so be careful.
 */

#ifndef TWEAKABLES_H
#define TWEAKABLES_H

#define MOVE_FACTOR 1

#define DEFAULT_UPDATE_TIME (30 * 60)  /* update time (minutes!) */
#define DEFAULT_RANDOM_UPDATE_RANGE 0  /* again, in minutes. */
#define DEFAULT_RANDOM_SEGMENT_RANGE 0 /* again, in minutes. */
#define MOVES_PER_UPDATE 3
/* If MOVES_PER_UPDATE is set to 1, there will be no movement segments */
/* between updates; the move is counted as part of the update. */
/* Set this to something higher to have evenly spaced movement segments. */

#define LOGIN_NAME_SIZE 64

#define NUM_TIMES_TO_WAIT_FOR_LOCK 200
#define NEUTRAL_FD 1000

#define MAXCOMMSTRSIZE 250
#define COMMANDSIZE 42
#define MAXARGS 256

#define RTEMP 0   /* regular temp for planet */
#define TEMP 1    /* temperature */
#define METHANE 2 /* %age of gases for terraforming */
#define OXYGEN 3
#define CO2 4
#define HYDROGEN 5
#define NITROGEN 6
#define SULFUR 7
#define HELIUM 8
#define OTHER 9
#define TOXIC 10

const char CHAR_CURR_SCOPE = ':'; /* for getplace */

const char CHAR_LAND = '*';
const char CHAR_SEA = '.';
const char CHAR_MOUNT = '^';
const char CHAR_DIFFOWNED = '?';
const char CHAR_PLATED = 'o';
const char CHAR_WASTED = '%';
const char CHAR_GAS = '~';
const char CHAR_CLOAKED = ' ';
const char CHAR_ICE = '#';
const char CHAR_CRYSTAL = 'x';
const char CHAR_DESERT = '-';
const char CHAR_FOREST = ')';

const char CHAR_MY_TROOPS = 'X';
const char CHAR_ALLIED_TROOPS = 'A';
const char CHAR_ATWAR_TROOPS = 'E';
const char CHAR_NEUTRAL_TROOPS = 'N';

/* 3.0 feature */
#define LIMITED_RESOURCES                      \
  1 /* set to 0 if you want unlimited resource \
       availability (pre 3.0)*/

#define NAMESIZE 18
#define RNAMESIZE 35
#define MOTTOSIZE 64
#define PERSONALSIZE 128
#define PLACENAMESIZE (NAMESIZE + NAMESIZE + 13)
#define NUMSTARS 256
#define MAXPLANETS 10
#undef MAXMOONS /*3*/

#define MAX_X 45 /* top range for planet */
#define MAX_Y 19
#define RATIOXY 3.70 /* map ratio between x and y */
                     /* ranges of map sizes (x usually ) */

#define UNIVSIZE 150000
#define SYSTEMSIZE 2000
#define PLORBITSIZE 50

#define WEEKLY 300
#define DAILY 180

#define VICTORY_PERCENT 10
#define VICTORY_UPDATES 5

#define AUTO_TELEG_SIZE 2000
#define UNTRANS_MSG "[ ? ]"
#undef TELEG_TRANS_APCOST /*1*/
#define TELEG_TRANS_RPCOST 5
#define TELEG_TRANS_AMT 0.45
#define TELEG_LETTERS 7
#define TELEG_PLAYER_AUTO (-2)
#define TELEG_MAX_AUTO 7 /* when changing, alter field in plinfo */
#define TELEG_DELIM '~'
#define TELEG_NDELIM "%[^~]"

#define MASS_FUEL 0.05
#define MASS_RESOURCE 0.1
#define MASS_DESTRUCT 0.15
#define MASS_ARMOR 1.0
#define MASS_SIZE 0.2
#define MASS_HANGER 0.1
#define MASS_GUNS 0.2

#define SIZE_GUNS 0.1
#define SIZE_CREW 0.01
#define SIZE_RESOURCE 0.02
#define SIZE_FUEL 0.01
#define SIZE_DESTRUCT 0.02
#define SIZE_HANGER 0.1

/* Constants for Factory mass and size */
#define HAB_FACT_SIZE 0.2

/* Cost factors for factory activation cost */
#define HAB_FACT_ON_COST 4
#define PLAN_FACT_ON_COST 2

#define SECTOR_DAMAGE 0.3
#define SHIP_DAMAGE 2.0

#define VN_RES_TAKE 0.5 /* amt of resource of a sector the VN's take */

#define LAUNCH_GRAV_MASS_FACTOR 0.18 /* fuel use modifier for taking off */
#define LAND_GRAV_MASS_FACTOR 0.0145

#define FUEL_GAS_ADD 5.0 /* amt of fuel to add to ea ships tanks */
#define FUEL_GAS_ADD_TANKER 100.0
#define FUEL_GAS_ADD_HABITAT 200.0
#define FUEL_GAS_ADD_STATION 100.0
#define FUEL_USE                               \
  0.02 /* fuel use per ship mass pt. per speed \
          factor */
#define HABITAT_PROD_RATE 0.05
#define HABITAT_POP_RATE 0.20

#define REPAIR_RATE 25.0 /* rate at which ships get repaired */
#define SECTOR_REPAIR_COST \
  10 /* how much it costs to remove a wasted status from a sector */
#define NATURAL_REPAIR                                  \
  5 /* chance of the wasted status being removed/update \
       */

#define CREAT_UNIV_ITERAT 10 /* iterations for star movement */

#define GRAV_FACTOR 0.0025 /* not sure as to what this should be*/

#define FACTOR_FERT_SUPPORT 1
/* # of people/fert pt sector supports*/
#define EFF_PROD 0.20               /* production of effcncy/pop*/
#define RESOURCE_PRODUCTION 0.00008 /* adjust these to change prod*/
#define FUEL_PRODUCTION 0.00008

#define DEST_PRODUCTION 0.00008
#define POPN_PROD 0.3

#define HYPER_DRIVE_READY_CHARGE 1
#define HYPER_DRIVE_FUEL_USE 5.0
#define HYPER_DIST_FACTOR 200.0

#define TECH_INVEST 0.01 /* invest factor */
#define TECH_SCALE 2.0   /* investment scale */

#define MOB_COST 0.00     /* mobiliz.c, doplanet.c cost/mob points*/
#undef RESOURCE_DEPLETION /*0.015 */
#define RESOURCE_DEPLETION 0.0
#define FACTOR_MOBPROD 0.06 /* mobilization production/person */
#define MESO_POP_SCALE 20000.0

#define FUEL_COST_TERRA 3.0  /* cost to terraform */
#define FUEL_COST_QUARRY 2.0 /* cost to mine resources */
#define FUEL_COST_PLOW 2.0
#define RES_COST_DOME 1
#define RES_COST_WPLANT 1
#define FUEL_COST_WPLANT 1.0

#define ENLIST_TROOP_COST 5 /* money it costs to pay a trooper */
#define UPDATE_TROOP_COST 1

#define PLAN_FIRE_LIM 20 /* max fire strength from planets */

#define TECH_SEE_STABILITY 15 /* min tech to see star stability */

#define TECH_EXPLORE 10 /* min tech to see your whole planet */

#define ENVIR_DAMAGE_TOX 70
/* min tox to damage planet */

#define PLANETGRAVCONST 0.05
#define SYSTEMGRAVCONST 150000.0

#define FUEL_MANEUVER 0.3 /* order.c-- fuel it costs to change aim */
#define DIST_TO_LAND 10.0 /* moveship.c,land.c -- */
#define DIST_TO_DOCK 10.0 /* changed to fix a bug. Maarten */
/* description: you could when you just entered planet scope assaault/dock
   with a ship in close orbit, and then immediately land. */

#undef DIST_TO_BURN /*50*/ /* distance from sun needed to destroy ship */

#define FACTOR_DAMAGE 2.0
#define FACTOR_DESTPLANET 0.35

/* various compiler options that may save cpu time/disk space */
#define NO_SLIDING_SCALE_AUTOMOVE 0 /* move to all four adjacent spots */
#define POPN_MOVE_SCALE_1 400       /* limit at which popn moves to all */
#define POPN_MOVE_SCALE_2 3000      /* " " " popn moves to 2 adj. spaces */
                                    /* otherwise move to only 1 adj. space*/
#define SHIP_MOVE_SCALE 3.0

/* to save object code */
#define getchr() fgetc(stdin)
#define putchr(c) fputc((c), stdout)
/* assorted macros */
/* sign,abs val of number */
#define sgn(x) (((x) >= 0) ? 1 : -1)
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#define MAX(x, y) (((x) < (y)) ? (y) : (x))
/* positive modulus */
#define mod(a, b, dum) ((dum) = (a) % (b), abs(dum))
/* euclidean distance */
#define Distsq(x1, y1, x2, y2) \
  (((x1) - (x2)) * ((x1) - (x2)) + ((y1) - (y2)) * ((y1) - (y2)))

/* look up sector */
#define Sector(pl, x, y) (Smap[(x) + (y) * (pl).Maxx])
/* adjust temperature to displayed */
#define Temp(x) ((int)(x))

/* number of AP's to add to each player in ea. system */
/*   (look in doturn)  */
#define LIMIT_APs 255 /* max # of APs you can have */

enum ptype_t {
  TYPE_EARTH,
  TYPE_ASTEROID,
  TYPE_MARS,
  TYPE_ICEBALL,
  TYPE_GASGIANT,
  TYPE_WATER,
  TYPE_FOREST,
  TYPE_DESERT
};

/* number of global APs each planet is worth */
#define EARTH_POINTS int_rand(5, 8)
#define ASTEROID_POINTS 1
#define MARS_POINTS int_rand(2, 3)
#define ICEBALL_POINTS int_rand(2, 3)
#define GASGIANT_POINTS int_rand(8, 20)
#define WATER_POINTS int_rand(2, 3)
#define FOREST_POINTS int_rand(2, 3)
#define DESERT_POINTS int_rand(2, 3)

#define SEA 0
#define LAND 1
#define MOUNT 2
#define GAS 3
#define ICE 4
#define FOREST 5
#define DESERT 6
#define PLATED 7
#define WASTED 8

#define CIV 0
#define MIL 1

#define MAX_SECT_POPN 32767

#define TOXMAX 20 /* max a toxwc can hold */

#define SIGBLOCKS (SIGHUP | SIGTERM | SIGINT | SIGQUIT | SIGSTOP | SIGTSTP)
/* signals to block... */

#define RESOURCE 0 /* for market */
#define DESTRUCT 1
#define FUEL 2
#define CRYSTAL 3

#define MERCHANT_LENGTH 200000.0
#define INCOME_FACTOR 0.002
#define INSURG_FACTOR 1
#define UP_BID 0.10

#define GUN_COST 1.00
#define CREW_COST 0.05
#define CARGO_COST 0.05
#define FUEL_COST 0.05
#define AMMO_COST 0.05
#define SPEED_COST 0.50
#define CEW_COST 0.003
#define ARMOR_COST 3.50
#define HANGER_COST 0.50

#define AFV_FUEL_COST 1.0

#define MECH_ATTACK 3.0

#define SPORE_SUCCESS_RATE 25

#define CLIENT_CHAR '|'

#define VICT_SECT 1000
#define VICT_SHIP 333
#define VICT_TECH .10
#define VICT_MORALE 200
#define VICT_RES 100
#define VICT_FUEL 15
#define VICT_MONEY 5
#define VICT_DIVISOR 10000

#define STRIKE_DISTANCE_FACTOR 5.5
#define COMPLEXITY_FACTOR \
  10.0 /* determines steepness of design complexity function */

#define REPEAT_CHARACTER                              \
  ' ' /* this character makes the previous command to \
         repeat */
#define MAXGOVERNORS 5
#define POD_THRESHOLD 18
#define POD_DECAY 4
#define AP_FACTOR \
  50.0 /* how planet size affects the rate of atmosphere processing */
#define DISSIPATE 80 /* updates to dissipate dust and gases */

#endif  // TWEAKABLES_H
