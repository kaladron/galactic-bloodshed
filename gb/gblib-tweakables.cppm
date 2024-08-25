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
export constexpr std::array<double, 3> MoveConsts = {600.0, 300.0, 50.0};
// amnt to move for each ship speed level (ordered)
export constexpr std::array<double, 10> SpeedConsts = {
    0.0, 0.61, 1.26, 1.50, 1.73, 1.81, 1.90, 1.93, 1.96, 1.97};
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

export constexpr double MASS_FUEL = 0.05;
export constexpr double MASS_RESOURCE = 0.1;
export constexpr double MASS_DESTRUCT = 0.15;
export constexpr double MASS_ARMOR = 1.0;
export constexpr double MASS_SIZE = 0.2;
export constexpr double MASS_HANGER = 0.1;
export constexpr double MASS_GUNS = 0.2;

export constexpr double SIZE_GUNS = 0.1;
export constexpr double SIZE_CREW = 0.01;
export constexpr double SIZE_RESOURCE = 0.02;
export constexpr double SIZE_FUEL = 0.01;
export constexpr double SIZE_DESTRUCT = 0.02;
export constexpr double SIZE_HANGER = 0.1;

// Constants for Factory mass and size
export constexpr double HAB_FACT_SIZE = 0.2;

// Cost factors for factory activation cost
export constexpr int HAB_FACT_ON_COST = 4;
export constexpr int PLAN_FACT_ON_COST = 2;

export constexpr double SECTOR_DAMAGE = 0.3;
export constexpr double SHIP_DAMAGE = 2.0;

export constexpr double VN_RES_TAKE =
    0.5;  // amt of resource of a sector the VN's take

export constexpr double REPAIR_RATE = 25.0;  // rate at which ships get repaired
export constexpr int SECTOR_REPAIR_COST =
    10;  // how much it costs to remove a wasted status from a sector
export constexpr int NATURAL_REPAIR =
    5;  // chance of the wasted status being removed/update

export constexpr double CREAT_UNIV_ITERAT = 10;  // iterations for star movement

export constexpr double GRAV_FACTOR =
    0.0025;  // not sure as to what this should be

export constexpr int FACTOR_FERT_SUPPORT =
    1;                                    // # of people/fert pt sector supports
export constexpr double EFF_PROD = 0.20;  // production of effcncy/pop
export constexpr double RESOURCE_PRODUCTION =
    0.00008;  // adjust these to change prod
export constexpr double FUEL_PRODUCTION = 0.00008;

export constexpr double DEST_PRODUCTION = 0.00008;
export constexpr double POPN_PROD = 0.3;

export constexpr double MOB_COST = 0.00;  // mobiliz.c, planet.c cost/mob points
export constexpr double RESOURCE_DEPLETION = 0.0;
export constexpr double FACTOR_MOBPROD =
    0.06;  // mobilization production/person
export constexpr double MESO_POP_SCALE = 20000.0;

export constexpr double FUEL_COST_TERRA = 3.0;   // cost to terraform
export constexpr double FUEL_COST_QUARRY = 2.0;  // cost to mine resources
export constexpr double FUEL_COST_PLOW = 2.0;
export constexpr int RES_COST_DOME = 1;
export constexpr int RES_COST_WPLANT = 1;
export constexpr double FUEL_COST_WPLANT = 1.0;

export constexpr int ENLIST_TROOP_COST = 5;  // money it costs to pay a trooper
export constexpr int UPDATE_TROOP_COST = 1;

export constexpr int PLAN_FIRE_LIM = 20;  // max fire strength from planets

export constexpr int TECH_SEE_STABILITY = 15;  // min tech to see star stability

export constexpr int TECH_EXPLORE = 10;  // min tech to see your whole planet

// min tox to damage planet
export constexpr int ENVIR_DAMAGE_TOX = 70;

export constexpr double PLANETGRAVCONST = 0.05;
export constexpr double SYSTEMGRAVCONST = 150000.0;

/* description: you could when you just entered planet scope assaault/dock
   with a ship in close orbit, and then immediately land. */
export constexpr double FACTOR_DAMAGE = 2.0;
export constexpr double FACTOR_DESTPLANET = 0.35;
