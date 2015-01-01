// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "ships.h"

/* has an on/off switch */
bool has_switch(shiptype *s) {
 return Shipdata[(s)->type][ABIL_HASSWITCH];
}


