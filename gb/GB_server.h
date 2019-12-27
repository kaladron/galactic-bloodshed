// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GB_SERVER_H
#define GB_SERVER_H

#include "gb/races.h"
#include "gb/ships.h"
#include "gb/vars.h"

void notify_race(const player_t, const std::string &);
bool notify(const player_t, const governor_t, const std::string &);
void d_think(const player_t, const governor_t, const std::string &);
void d_broadcast(const player_t, const governor_t, const std::string &);
void d_shout(const player_t, const governor_t, const std::string &);
void d_announce(const player_t, const governor_t, const starnum_t,
                const std::string &);
void do_next_thing(Db &);
void kill_ship(int, Ship *);
void compute_power_blocks();
void insert_sh_univ(struct stardata *, Ship *);
void insert_sh_star(startype *, Ship *);
void insert_sh_plan(Planet &, Ship *);
void insert_sh_ship(Ship *, Ship *);
void remove_sh_star(Ship &);
void remove_sh_plan(Ship &);
void remove_sh_ship(Ship &, Ship &);
void warn_race(const player_t, const std::string &);
void warn(const player_t, const governor_t, const std::string &);
void warn_star(const player_t, const starnum_t, const std::string &);
void notify_star(const player_t, const governor_t, const starnum_t,
                 const std::string &);
void adjust_morale(Race *, Race *, int);

using segments_t = uint32_t;

extern time_t next_update_time; /* When will next update be... approximately */
extern time_t
    next_segment_time; /* When will next segment be... approximately */
extern unsigned int update_time;  /* Interval between updates in minutes */
extern segments_t nsegments_done; /* How many movements have we done so far? */

extern const char *Desnames[];
extern const char Dessymbols[];

#endif  // GB_SERVER_H
