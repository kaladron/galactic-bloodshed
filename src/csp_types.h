/*
 * csp_types.h: Variables and defines used internally in CSP related functions
 *
 * CSP, copyright (c) 1993 by John P. Deragon, Evan Koffler
 *
 * Please send any modifications of this file to:
 *    evank@netcom.com
 *    deragon@jethro.nyu.edu
 * #ident  "@(#)csp_types.h	1.3 12/1/93 "
 *
 */
#ifndef _CSP_TYPES_H_
#define _CSP_TYPES_H_

/* character to be found in column 0 of a CSP output line */
/* sent by server to client */
#define CSP_CLIENT '|'

/* identifier sent by client to server when sending a command/request */
#define CSP_SERVER "CSP"

/* Maximum depth of scope ie: nested ships */
#define CSPD_MAXSHIP_SCOPE 10
#define CSPD_NOSHIP 0

/* Maximum number of unique letters in star and planet names */
#define MAX_SCOPE_LTRS 4

/* Levels for sending the prompt used in CSP_PROMPT */
enum LOCATION {
  CSPD_UNIV, /* 0 */
  CSPD_STAR, /* 1 */
  CSPD_PLAN, /* 2 */
  CSPD_LOCATION_UNKNOWN = 99
};

/* used in CSP_PROFILE among other places */
enum RACE_TYPE {
  CSPD_RACE_UNKNOWN, /* 0 */
  CSPD_RACE_MORPH,   /* 1 */
  CSPD_RACE_NORMAL   /* 2 */
};

/* used in CSP_PROFILE and CSP_RELATION */
enum RELATION {
  CSPD_RELAT_UNKNOWN, /* 0 */
  CSPD_RELAT_ALLIED,  /* 1 */
  CSPD_RELAT_NEUTRAL, /* 2 */
  CSPD_RELAT_WAR      /* 3 */
};

/* used in CSP_MAP */
enum TROOPS {
  CSPD_TROOPS_UNKNOWN,
  CSPD_TROOPS_ALLIED,
  CSPD_TROOPS_NEUTRAL,
  CSPD_TROOPS_ENEMY,
  CSPD_TROOPS_MINE
};

/* used in CSP_PROFILE for discoveries */
enum DISCOVERIES {
  CSPD_HYPERDRIVE, /* 0 */
  CSPD_CRYSTAL,    /* 1 */
  CSPD_LASER,      /* 2 */
  CSPD_CEW,        /* 3 */
  CSPD_AVPM,       /* 4 */
  CSPD_MAX_NUM_DISCOVERY,
  CSPD_DISCOVERY_UNKNOWN = 99
};

enum PLAYER_TYPE {
  CSPD_NORMAL, /* 0 */
  CSPD_DIETY,  /* 1 */
  CSPD_GUEST,  /* 2 */
};

enum SECTOR_TYPES {
  CSPD_SECTOR_SEA,
  CSPD_SECTOR_LAND,
  CSPD_SECTOR_MOUNT,
  CSPD_SECTOR_GAS,
  CSPD_SECTOR_ICE,
  CSPD_SECTOR_FOREST,
  CSPD_SECTOR_DESERT,
  CSPD_SECTOR_PLATED,
  CSPD_SECTOR_WASTED,
  CSPD_SECTOR_UNKNOWN
};

enum PLANET_TYPES {
  CSPD_PLANET_CLASS_M,
  CSPD_PLANET_ASTEROID,
  CSPD_PLANET_AIRLESS,
  CSPD_PLANET_ICEBALL,
  CSPD_PLANET_JOVIAN,
  CSPD_PLANET_WATERBALL,
  CSPD_PLANET_FOREST,
  CSPD_PLANET_DESERT
};

enum COMMUNICATION {
  CSPD_BROADCAST,
  CSPD_ANNOUNCE,
  CSPD_THINK,
  CSPD_SHOUT,
  CSPD_EMOTE
};

#define CSPD_XTAL_SYMBOL 'x'
#define CSPD_TROOP_MINE_SYMBOL 'X'
#define CSPD_TROOP_ALLIED_SYMBOL 'A'
#define CSPD_TROOP_ENEMY_SYMBOL 'E'
#define CSPD_TROOP_NEUTRAL_SYMBOL 'N'

#endif
/* _CSP_TYPES_H_ */
