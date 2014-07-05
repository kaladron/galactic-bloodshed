/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB.c, enroll.dat.
 * Restrictions in GB.c.
 */

/* for power report */

struct power {
  unsigned long troops;   /* total troops */
  unsigned long popn;     /* total population */
  unsigned long resource; /* total resource in stock */
  unsigned long fuel;
  unsigned long destruct;     /* total dest in stock */
  unsigned short ships_owned; /* # of ships owned */
  unsigned short planets_owned;
  unsigned long sectors_owned;
  unsigned long money;
  unsigned long sum_mob; /* total mobilization */
  unsigned long sum_eff; /* total efficiency */
};

EXTERN struct power Power[MAXPLAYERS];
