// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* csp.h: Contains the defines for all the Client-Server Protocol (CSP)
          numbers. */

#ifndef CSP_H
#define CSP_H

/*---- Server responses ----*/

/* VERSION */
#define CSP_VERSION_INFO 10    /* version XX */
#define CSP_VERSION_OPTIONS 11 /* options set YY */

/* LOGIN */
#define CSP_CLIENT_ON 30  /* client mode on      */
#define CSP_CLIENT_OFF 31 /* client mode off     */
#define CSP_KNOWLEDGE 32

/* INFO */
#define CSP_SCOPE_PROMPT 40

/* UPDATE/SEGMENT/RESET */
#define CSP_UPDATE_START 50  /* update started */
#define CSP_UPDATE_END 51    /* update finshed */
#define CSP_SEGMENT_START 52 /* segment started */
#define CSP_SEGMENT_END 53   /* segment finished */
#define CSP_RESET_START 54   /* reset started */
#define CSP_RESET_END 55     /* reset finished */
#define CSP_BACKUP_START 56  /* backup started */
#define CSP_BACKUP_END 57    /* backup finished */

#define CSP_UPDATES_SUSPENDED 58 /* updates suspended */
#define CSP_UPDATES_RESUMED 59   /* updates resumed */

/* SURVEY */
#define CSP_SURVEY_INTRO 101  /* planet info */
#define CSP_SURVEY_SECTOR 102 /* sector info */
#define CSP_SURVEY_END 103    /* end of command(EOC) */

/* RELATION */
#define CSP_RELATION_INTRO 201 /* race # & name */
#define CSP_RELATION_DATA 202  /* the data */
#define CSP_RELATION_END 203   /* end of command(EOC) */

/* PROFILE */
/* DYNAMIC =  Active Knowledge Capital Morale Gun GTele STele */
/* DYNAMIC_OTHER = %Know Morale Gun GTele OTele SecPref */
#define CSP_PROFILE_INTRO 301 /* header */
#define CSP_PROFILE_PERSONAL 302
#define CSP_PROFILE_DYNAMIC 303
#define CSP_PROFILE_DYNAMIC_OTHER 304
#define CSP_PROFILE_RACE_STATS 305
#define CSP_PROFILE_RACE_STATS_OTHER 306
#define CSP_PROFILE_PLANET 307
#define CSP_PROFILE_SECTOR 308
#define CSP_PROFILE_DISCOVERY 309 /* discoveries */
#define CSP_PROFILE_END 310

/* WHO */
#define CSP_WHO_INTRO 401   /* header */
#define CSP_WHO_DATA 402    /* actual data */
#define CSP_WHO_COWARDS 403 /* send either # of cowards */
#define CSP_WHO_END 403     /* or WHO_END as terminator */

/* EXPLORE */
/* STAR_DATA = # Name Ex Inhab Auto Slaved Toxic Compat #Sec Depo Xtal Type */
#define CSP_EXPLORE_INTRO 501
#define CSP_EXPLORE_STAR 502
#define CSP_EXPLORE_STAR_ALIENS 503
#define CSP_EXPLORE_STAR_DATA 504
#define CSP_EXPLORE_STAR_END 505
#define CSP_EXPLORE_END 506

/* MAP */
/* DYNAMIC_1 = Type Sects Guns MobPoints Res Des Fuel Xtals */
/* DYNAMIC_2 = Mob AMob Compat Pop ^Pop ^TPop Mil Tax ATax Deposits Est Prod */
#define CSP_MAP_INTRO 601
#define CSP_MAP_DYNAMIC_1 602
#define CSP_MAP_DYNAMIC_2 603
#define CSP_MAP_ALIENS 604
#define CSP_MAP_DATA 605
#define CSP_MAP_END 606

/* CLIENT GENERATED COMMANDED */
#define CSP_LOGIN_COMMAND 1101    /* login command   */
#define CSP_VERSION_COMMAND 1102  /* version command */
#define CSP_SURVEY_COMMAND 1103   /* imap command    */
#define CSP_RELATION_COMMAND 1104 /* relation command */
#define CSP_PROFILE_COMMAND 1105  /* profile command */
#define CSP_WHO_COMMAND 1106      /* who 	command */
#define CSP_EXPLORE_COMMAND 1107  /* exploration command */
#define CSP_MAP_COMMAND 1108      /* map command */
#define CSP_SCOPE_COMMAND 1110    /* request a prompt */

/* Dan Dickey for XGB ** NOT SUPPORTED FULLY ** */

#define CSP_ORBIT_COMMAND 1501    /* orbit command */
#define CSP_ZOOM_COMMAND 1502     /* zoom command */
#define CSP_PLANDUMP_COMMAND 1503 /* planet dump command */
#define CSP_SHIPDUMP_COMMAND 1504 /* ship dump command */

/* Planet Dumps */
#define CSP_PLANDUMP_INTRO 2000      /* planet name */
#define CSP_PLANDUMP_CONDITIONS 2001 /* conditions */
#define CSP_PLANDUMP_STOCK 2002      /* stockpiles */
#define CSP_PLANDUMP_PROD 2003       /* production last update */
#define CSP_PLANDUMP_MISC 2004       /* rest of stuff */
#define CSP_PLANDUMP_NOEXPL 2005     /* planet not explored */

/* General usage */
#define CSP_STAR_UNEXPL 2010 /* star is not explored */
/* ORBIT */
#define CSP_ORBIT_OUTPUT_INTRO 2020  /* orbit parameters */
#define CSP_ORBIT_STAR_DATA 2021     /* star info */
#define CSP_ORBIT_UNEXP_PL_DATA 2022 /* unexplored planet info */
#define CSP_ORBIT_EXP_PL_DATA 2023   /* explored planet info */
#define CSP_ORBIT_SHIP_DATA 2024     /* ship info */
#define CSP_ORBIT_OUTPUT_END 2025    /* end of command(EOC) */

/* Ship Dumps */
#define CSP_SHIPDUMP_GEN 2030         /* General information */
#define CSP_SHIPDUMP_STOCK 2031       /* Stock information */
#define CSP_SHIPDUMP_STATUS 2032      /* Status information */
#define CSP_SHIPDUMP_WEAPONS 2033     /* Weapons information */
#define CSP_SHIPDUMP_FACTORY 2034     /* Factory information */
#define CSP_SHIPDUMP_DEST 2035        /* Destination information */
#define CSP_SHIPDUMP_PTACT_GEN 2036   /* General planet tactical */
#define CSP_SHIPDUMP_PTACT_PDIST 2037 /* distance between planet's */
#define CSP_SHIPDUMP_STACT_PDIST 2038 /* distance between a ship a*/
#define CSP_SHIPDUMP_PTACT_INFO 2039  /* for a ship from a planet */
#define CSP_SHIPDUMP_STACT_INFO 2040  /* ifor a ship from a ship */
#define CSP_SHIPDUMP_ORDERS 2041      /* Ship orders */
#define CSP_SHIPDUMP_THRESH 2042      /* Ship threshloading */
#define CSP_SHIPDUMP_SPECIAL 2043     /* Ship specials */
#define CSP_SHIPDUMP_HYPER 2044       /* Hyper drive usage */
#define CSP_SHIPDUMP_END 2055         /* end of command (EOC) */

/* Dan Dickey for XGB ** NOT SUPPORTED FULLY ** */

/*---- Error Responses ----*/
#define CSP_ERR 9900                 /* error           */
#define CSP_ERR_TOO_MANY_ARGS 9901   /* too many args   */
#define CSP_ERR_TOO_FEW_ARGS 9902    /* too few args    */
#define CSP_ERR_UNKNOWN_COMMAND 9903 /* unknown command */
#define CSP_ERR_NOSUCH_PLAYER 9904   /* no such player  */
#define CSP_ERR_NOSUCH_PLACE 9905    /* no such place - scope err  */

#define CSP_MAX_SERVER_COMMAND 2000

#define CSP_ZOOM 35

#endif  // CSP_H
