// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GB_SERVER_H
#define GB_SERVER_H

void do_next_thing(Db &);
void compute_power_blocks();
void insert_sh_univ(stardata *, Ship *);
void insert_sh_star(Star &, Ship *);
void insert_sh_plan(Planet &, Ship *);
void insert_sh_ship(Ship *, Ship *);
void remove_sh_star(Ship &);
void remove_sh_plan(Ship &);
void remove_sh_ship(Ship &, Ship &);

extern time_t next_update_time; /* When will next update be... approximately */
extern time_t
    next_segment_time; /* When will next segment be... approximately */
extern unsigned int update_time;  /* Interval between updates in minutes */
extern segments_t nsegments_done; /* How many movements have we done so far? */

extern const char *Desnames[];
extern const char Dessymbols[];

#endif  // GB_SERVER_H
