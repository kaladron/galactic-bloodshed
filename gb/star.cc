// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module gblib;

int Star::control(player_t Playernum, governor_t Governor) {
  return (!Governor || star_struct.governor[Playernum - 1] == Governor);
}
