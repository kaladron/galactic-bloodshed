// SPDX-License-Identifier: Apache-2.0

export module gblib:tweakables;

import :types;

// Shipping routes - DON'T change this unless you know what you are doing
export inline constexpr int MAX_ROUTES = 4;

// Number of AP's to add to each player in each system.
export const ap_t LIMIT_APs = 255;  // max # of APs you can have

export constexpr char CHAR_LAND = '*';
export constexpr char CHAR_SEA = '.';
export constexpr char CHAR_MOUNT = '^';
export constexpr char CHAR_DIFFOWNED = '?';
export constexpr char CHAR_PLATED = 'o';
export constexpr char CHAR_WASTED = '%';
export constexpr char CHAR_GAS = '~';
export constexpr char CHAR_CLOAKED = ' ';
export constexpr char CHAR_ICE = '#';
export constexpr char CHAR_CRYSTAL = 'x';
export constexpr char CHAR_DESERT = '-';
export constexpr char CHAR_FOREST = ')';

export constexpr char CHAR_MY_TROOPS = 'X';
export constexpr char CHAR_ALLIED_TROOPS = 'A';
export constexpr char CHAR_ATWAR_TROOPS = 'E';
export constexpr char CHAR_NEUTRAL_TROOPS = 'N';

// Number of global APs each planet is worth
export constexpr ap_t ASTEROID_POINTS = 1;

export constexpr int MAXPLAYERS = 64;
export constexpr int NUM_DISCOVERIES = 80;

export constexpr int NAMESIZE = 18;
export constexpr int RNAMESIZE = 35;
export constexpr int MOTTOSIZE = 64;
export constexpr int PERSONALSIZE = 128;
export constexpr int PLACENAMESIZE = (NAMESIZE + NAMESIZE + 13);
export constexpr int NUMSTARS = 256;
export constexpr int MAXPLANETS = 10;
export constexpr int MAXGOVERNORS = 5u;
