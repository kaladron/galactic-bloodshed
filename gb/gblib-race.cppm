// SPDX-License-Identifier: Apache-2.0

export module gblib:race;

import :types;

#include "gb/tweakables.h"

using toggletype = struct {
  char invisible;
  char standby;
  char color; /* 1 if you are using a color client */
  char gag;
  char double_digits;
  char inverse;
  char geography;
  char autoload;
  player_t highlight; /* which race to highlight */
  char compat;
};

export class Race {
 public:
  player_t Playernum;
  char name[RNAMESIZE]; /* Racial name. */
  char password[RNAMESIZE];
  char info[PERSONALSIZE];     /* personal information */
  char motto[MOTTOSIZE];       /* for a cute message */
  unsigned char absorb;        /* Does this race absorb enemies in combat? */
  unsigned char collective_iq; /* Does this race have collective IQ? */
  unsigned char pods;          /* Can this race use pods? */
  unsigned char fighters;      /* Fight rating of this race. */
  unsigned char IQ;
  unsigned char IQ_limit; /* Asymtotic IQ for collective IQ races. */
  unsigned char number_sexes;
  unsigned char fertilize; /* Chance that this race will increase the
                              fertility of its sectors by 1 each update */
  double adventurism;
  double birthrate;
  double mass;
  double metabolism;
  short conditions[OTHER + 1]; /* Atmosphere/temperature this race likes. */
  double likes[SectorType::SEC_WASTED + 1]; /* Sector condition compats. */
  unsigned char likesbest; /* 100% compat sector condition for this race. */

  char dissolved; /* Player has quit. */
  char God;       /* Player is a God race. */
  char Guest;     /* Player is a guest race. */
  char Metamorph; /* Player is a morph; (for printing). */
  char monitor;
  /* God is monitering this race. */  // TODO(jeffbailey): Remove this.

  char translate[MAXPLAYERS]; /* translation mod for each player */

  uint64_t atwar;
  uint64_t allied;

  shipnum_t Gov_ship;                /* Shipnumber of government ship. */
  long morale;                       /* race's morale level */
  unsigned int points[MAXPLAYERS];   /* keep track of war status against
                                        another player - for short reports */
  unsigned short controlled_planets; /* Number of planets under control. */
  unsigned short victory_turns;
  unsigned short turn;

  double tech;
  unsigned char discoveries[NUM_DISCOVERIES]; /* Tech discoveries. */
  unsigned long victory_score;                /* Number of victory points. */
  bool votes;
  ap_t planet_points; /* For the determination of global APs */

  char governors;
  struct gov {
    char name[RNAMESIZE];
    char password[RNAMESIZE];
    unsigned char active;
    ScopeLevel deflevel;
    unsigned char defsystem;
    unsigned char defplanetnum; /* current default */
    ScopeLevel homelevel;
    unsigned char homesystem;
    unsigned char homeplanetnum; /* home place */
    unsigned long newspos[4];    /* news file pointers */
    toggletype toggle;
    money_t money;
    unsigned long income;
    money_t maintain;
    unsigned long cost_tech;
    unsigned long cost_market;
    unsigned long profit_market;
    time_t login; /* last login for this governor */
  } governor[MAXGOVERNORS + 1];
};