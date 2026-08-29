// SPDX-License-Identifier: Apache-2.0

/// \file star.cc
/// \brief Star class member functions.

module gblib;

bool Star::control(player_t Playernum, governor_t Governor) const {
  return (Governor == 0 ||
          star_struct.governor[Playernum.value - 1] == Governor);
}
