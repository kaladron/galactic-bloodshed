// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GB_SERVER_H
#define GB_SERVER_H

#include <string>
#include "races.h"
#include "ships.h"
#include "vars.h"

void notify_race(const player_t, const std::string &);
bool notify(const player_t, const governor_t, const std::string &);
void d_think(player_t, governor_t, const std::string &);
void d_broadcast(player_t, governor_t, const std::string &);
void d_shout(player_t, governor_t, const std::string &);
void d_announce(player_t, governor_t, starnum_t, const std::string &);
void do_next_thing(void);
void check_for_telegrams(int, int);
void kill_ship(int, shiptype *);
void compute_power_blocks(void);
void insert_sh_univ(struct stardata *, shiptype *);
void insert_sh_star(startype *, shiptype *);
void insert_sh_plan(Planet *, shiptype *);
void insert_sh_ship(shiptype *, shiptype *);
void remove_sh_star(shiptype *);
void remove_sh_plan(shiptype *);
void remove_sh_ship(shiptype *, shiptype *);
int ShipCompare(const void *, const void *);
void SortShips(void);
void warn_race(const player_t, const std::string &);
void warn(const player_t, const governor_t, const std::string &);
void warn_star(const player_t, const starnum_t, const std::string &);
void notify_star(const player_t, const governor_t, const starnum_t,
                 const std::string &);
void adjust_morale(racetype *, racetype *, int);

typedef uint32_t segments_t;

extern time_t next_update_time; /* When will next update be... approximately */
extern time_t
    next_segment_time; /* When will next segment be... approximately */
extern unsigned int update_time;  /* Interval between updates in minutes */
extern segments_t nsegments_done; /* How many movements have we done so far? */

extern const char *Desnames[];
extern const char Dessymbols[];

extern long clk;

#endif  // GB_SERVER_H
