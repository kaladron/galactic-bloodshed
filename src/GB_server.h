// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GB_SERVER_H
#define GB_SERVER_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void notify_race(int, const char *);
int notify(int, int, const char *);
void d_think(int, int, char *);
void d_broadcast(int, int, char *);
void d_shout(int, int, char *);
void d_announce(int, int, int, char *);
void do_next_thing(void);
void load_race_data(void);
void load_star_data(void);
void GB_time(int, int);
void GB_schedule(int, int);
void check_for_telegrams(int, int);
void kill_ship(int, shiptype *);
void compute_power_blocks(void);
void insert_sh_univ(struct stardata *, shiptype *);
void insert_sh_star(startype *, shiptype *);
void insert_sh_plan(planettype *, shiptype *);
void insert_sh_ship(shiptype *, shiptype *);
void remove_sh_star(shiptype *);
void remove_sh_plan(shiptype *);
void remove_sh_ship(shiptype *, shiptype *);
int ShipCompare(const void *, const void *);
void SortShips(void);
void warn_race(int, char *);
void warn(int, int, char *);
void warn_star(int, int, int, char *);
void notify_star(int, int, int, int, char *);
void post_star(char *, int, int);
void adjust_morale(racetype *, racetype *, int);

typedef uint32_t segments_t;

extern time_t next_update_time; /* When will next update be... approximately */
extern time_t
next_segment_time; /* When will next segment be... approximately */
extern unsigned int update_time;  /* Interval between updates in minutes */
extern segments_t nsegments_done; /* How many movements have we done so far? */

extern const char *Desnames[];
extern const char Dessymbols[];

extern racetype *Race;

extern long clk;

#endif // GB_SERVER_H
