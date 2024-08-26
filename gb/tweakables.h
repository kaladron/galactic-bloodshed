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

#define WELCOME_FILE "welcome.txt"
#define HELP_FILE DOCDIR "help.txt"
#define LEAVE_MESSAGE "\n*** Thank you for playing Galactic Bloodshed ***\n"

#undef EXTERNAL_TRIGGER /* if you wish to allow the below passwords to \
                             trigger updates and movement segments */
#ifdef EXTERNAL_TRIGGER
#define UPDATE_PASSWORD "put_your_update_password_here"
#define SEGMENT_PASSWORD "put_your_segment_password_here"
#endif

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

/* assorted macros */
// define MIN(x, y) (((x) > (y)) ? (y) : (x))
// define MAX(x, y) (((x) < (y)) ? (y) : (x))

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

#endif  // TWEAKABLES_H
