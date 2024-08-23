// SPDX-License-Identifier: Apache-2.0

export module gblib:tweakables;

import :types;

export constexpr bool VICTORY = false;       // Use victory conditions
export constexpr bool SHOW_COWARDS = false;  // Show number of invisible players
export constexpr bool POD_TERRAFORM =
    false;                              // Pods will terraform infected sectors
export constexpr bool VOTING = true;    // Allow player voting
export constexpr bool DISSOLVE = true;  // Allow players to dissolve
export constexpr bool DEFENSE = true;   // Allow planetary guns
export constexpr bool MARKET = true;    // Enable the market

// Shipping routes - DON'T change this unless you know what you are doing
export constexpr int MAX_ROUTES = 4;

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

export constexpr double TECH_INVEST = 0.01;  // invest factor

export constexpr int MAX_CRYSTALS = 127;

export constexpr double UNIVSIZE = 150000;
export constexpr double SYSTEMSIZE = 2000;
export constexpr double PLORBITSIZE = 50;

/* amount to move for each dir level. I arrived on these #'s only after
        hours of dilligent tweaking */
// amount to move for each directory level
export constexpr double MoveConsts[] = {600.0, 300.0, 50.0};
// amnt to move for each ship speed level (ordered)
export constexpr double SpeedConsts[] = {0.0,  0.61, 1.26, 1.50, 1.73,
                                         1.81, 1.90, 1.93, 1.96, 1.97};
// amount of fuel it costs to move at speed level

export constexpr char HYPER_DRIVE_READY_CHARGE = 1;
export constexpr double HYPER_DRIVE_FUEL_USE = 5.0;
export constexpr double HYPER_DIST_FACTOR = 200.0;

export constexpr double SHIP_MOVE_SCALE = 3.0;

export constexpr double FUEL_MANEUVER = 0.3;  // Fuel it costs to change aim

export constexpr double DIST_TO_LAND = 10.0;
export constexpr double DIST_TO_DOCK = 10.0;

export constexpr double LAUNCH_GRAV_MASS_FACTOR =
    0.18;  // fuel use modifier for taking off
export constexpr double LAND_GRAV_MASS_FACTOR = 0.0145;

export constexpr double FUEL_GAS_ADD =
    5.0;  // amt of fuel to add to ea ships tanks
export constexpr double FUEL_GAS_ADD_TANKER = 100.0;
export constexpr double FUEL_GAS_ADD_HABITAT = 200.0;
export constexpr double FUEL_GAS_ADD_STATION = 100.0;
export constexpr double FUEL_USE = 0.02; /* fuel use per ship mass pt. per speed
                                            factor */
export constexpr double HABITAT_PROD_RATE = 0.05;
export constexpr double HABITAT_POP_RATE = 0.20;