// SPDX-License-Identifier: Apache-2.0

export module gblib:universe;

import :types;
import :tweakables;
import std;

// Underlying universe-level singleton data structure
// This was previously called "stardata" but that name was confusing
// as it contains universe-wide data, not star-specific data
export struct universe_struct {
  int id{
      0};  // Universe ID for database persistence (always 1 for singleton)
  unsigned short numstars{0}; /* Total # of stars in universe */
  unsigned short ships{0};    /* Head of universe-wide ship list */
  ap_t AP[MAXPLAYERS]{};      /* Action points for each player */
  unsigned short VN_hitlist[MAXPLAYERS]{};
  /* # of ships destroyed by each player */
  int VN_index1[MAXPLAYERS]{}; /* negative value is used */
  int VN_index2[MAXPLAYERS]{}; /* VN's record of destroyed ships
                                          systems where they bought it */
};

// Wrapper class for Universe data (like Star wraps star_struct)
// Provides type-safe accessor methods instead of raw array access
export class Universe {
  universe_struct* data;

 public:
  explicit Universe(universe_struct& s) : data(&s) {}

  // Basic accessors
  unsigned short numstars() const { return data->numstars; }
  void set_numstars(unsigned short value) { data->numstars = value; }

  unsigned short ships() const { return data->ships; }
  void set_ships(unsigned short value) { data->ships = value; }

  // Action Point (AP) methods
  ap_t get_AP(player_t p) const {
    if (p < 1 || p > MAXPLAYERS) return 0;
    return data->AP[p - 1];
  }

  void set_AP(player_t p, ap_t value) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->AP[p - 1] = value;
  }

  void deduct_AP(player_t p, ap_t amount) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->AP[p - 1] = (data->AP[p - 1] > amount) ? (data->AP[p - 1] - amount)
                                                  : 0;
  }

  void add_AP(player_t p, ap_t amount) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->AP[p - 1] += amount;
  }

  // VN (Von Neumann) tracking methods
  unsigned short get_VN_hitlist(player_t p) const {
    if (p < 1 || p > MAXPLAYERS) return 0;
    return data->VN_hitlist[p - 1];
  }

  void set_VN_hitlist(player_t p, unsigned short value) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->VN_hitlist[p - 1] = value;
  }

  void increment_VN_hitlist(player_t p) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->VN_hitlist[p - 1]++;
  }

  void decrement_VN_hitlist(player_t p) {
    if (p < 1 || p > MAXPLAYERS) return;
    if (data->VN_hitlist[p - 1] > 0) data->VN_hitlist[p - 1]--;
  }

  int get_VN_index1(player_t p) const {
    if (p < 1 || p > MAXPLAYERS) return 0;
    return data->VN_index1[p - 1];
  }

  void set_VN_index1(player_t p, int value) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->VN_index1[p - 1] = value;
  }

  int get_VN_index2(player_t p) const {
    if (p < 1 || p > MAXPLAYERS) return 0;
    return data->VN_index2[p - 1];
  }

  void set_VN_index2(player_t p, int value) {
    if (p < 1 || p > MAXPLAYERS) return;
    data->VN_index2[p - 1] = value;
  }

  // Direct access to underlying struct (for migration compatibility)
  universe_struct* operator->() { return data; }
  const universe_struct* operator->() const { return data; }
  universe_struct& operator*() { return *data; }
  const universe_struct& operator*() const { return *data; }
};
