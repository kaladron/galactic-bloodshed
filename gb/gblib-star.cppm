// SPDX-License-Identifier: Apache-2.0

export module gblib:star;

import :types;
import :tweakables;

export struct Star {
  unsigned short ships;            /* 1st ship in orbit */
  char name[NAMESIZE];             /* name of star */
  governor_t governor[MAXPLAYERS]; /* which subordinate maintains the system */
  ap_t AP[MAXPLAYERS];             /* action pts alotted */
  uint64_t explored;               /* who's been here 64 bits*/
  uint64_t inhabited;              /* who lives here now 64 bits*/
  double xpos, ypos;

  unsigned char numplanets;            /* # of planets in star system */
  char pnames[MAXPLANETS][NAMESIZE];   /* names of planets */
  unsigned long planetpos[MAXPLANETS]; /* file posns of planets */

  unsigned char stability;   /* how close to nova it is */
  unsigned char nova_stage;  /* stage of nova */
  unsigned char temperature; /* factor which expresses how hot the star is*/
  double gravity;            /* attraction of star in "Standards". */

  starnum_t star_id;
  long dummy[1]; /* dummy bits for development */
};

/* this data will all be read at once */
export struct stardata {
  unsigned short numstars; /* # of stars */
  unsigned short ships;    /* 1st ship in orbit */
  ap_t AP[MAXPLAYERS];     /* Action pts for each player */
  unsigned short VN_hitlist[MAXPLAYERS];
  /* # of ships destroyed by each player */
  char VN_index1[MAXPLAYERS]; /* negative value is used */
  char VN_index2[MAXPLAYERS]; /* VN's record of destroyed ships
                                        systems where they bought it */
  unsigned long dummy[2];
};

export int control(const Star &, player_t, governor_t);
