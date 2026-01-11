// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module gblib;

int Star::control(player_t Playernum, governor_t Governor) const {
  return (Governor == 0 ||
          star_struct.governor[Playernum.value - 1] == Governor);
}
