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

#endif  // GB_SERVER_H
