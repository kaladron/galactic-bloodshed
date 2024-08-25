// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TWEAKABLES_H
#define TWEAKABLES_H

#define VERS "v5.2 10/9/92" /* don't change this */

#define GB_HOST "solana.mps.ohio-state.edu" /* change this for your machine */
#define GB_PORT 2010 /* change this for your port selection */

#define COMMAND_TIME_MSEC 250 /* time slice length in milliseconds */
#define COMMANDS_PER_TIME            \
  1 /* commands per time slice after \
       burst */
#define COMMAND_BURST_SIZE                                \
  250                        /* commands allowed per user \
                                in a burst */
#define DISCONNECT_TIME 7200 /* maximum idle time */
#define MAX_OUTPUT 32768     /* don't change this */

#define WELCOME_FILE "welcome.txt"
#define HELP_FILE DOCDIR "help.txt"
#define LEAVE_MESSAGE "\n*** Thank you for playing Galactic Bloodshed ***\n"

#undef EXTERNAL_TRIGGER /* if you wish to allow the below passwords to \
                             trigger updates and movement segments */
#ifdef EXTERNAL_TRIGGER
#define UPDATE_PASSWORD "put_your_update_password_here"
#define SEGMENT_PASSWORD "put_your_segment_password_here"
#endif

#define MOVE_FACTOR 1

#define DEFAULT_UPDATE_TIME (30 * 60)  /* update time (minutes!) */
#define DEFAULT_RANDOM_UPDATE_RANGE 0  /* again, in minutes. */
#define DEFAULT_RANDOM_SEGMENT_RANGE 0 /* again, in minutes. */
#define MOVES_PER_UPDATE 3
/* If MOVES_PER_UPDATE is set to 1, there will be no movement segments */
/* between updates; the move is counted as part of the update. */
/* Set this to something higher to have evenly spaced movement segments. */

#define LOGIN_NAME_SIZE 64

#define COMMANDSIZE 42
#define MAXARGS 256

#define MAX_X 45 /* top range for planet */
#define MAX_Y 19
#define RATIOXY 3.70 /* map ratio between x and y */
                     /* ranges of map sizes (x usually ) */

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

/* assorted macros */
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#define MAX(x, y) (((x) < (y)) ? (y) : (x))

/* number of global APs each planet is worth */
#define EARTH_POINTS int_rand(5, 8)
// Moved to module:
// constexpr ap_t ASTEROID_POINTS = 1;
#define MARS_POINTS int_rand(2, 3)
#define ICEBALL_POINTS int_rand(2, 3)
#define GASGIANT_POINTS int_rand(8, 20)
#define WATER_POINTS int_rand(2, 3)
#define FOREST_POINTS int_rand(2, 3)
#define DESERT_POINTS int_rand(2, 3)

#define CIV 0
#define MIL 1

#define MAX_SECT_POPN 32767

#define TOXMAX 20 /* max a toxwc can hold */

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
#define POD_THRESHOLD 18
#define POD_DECAY 4
#define AP_FACTOR \
  50.0 /* how planet size affects the rate of atmosphere processing */
#define DISSIPATE 80 /* updates to dissipate dust and gases */

#endif  // TWEAKABLES_H
