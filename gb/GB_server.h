// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef GB_SERVER_H
#define GB_SERVER_H

void do_next_thing(EntityManager&,
                   void*);  // void* is actually SessionRegistry*
void compute_power_blocks(EntityManager&);

#endif  // GB_SERVER_H
