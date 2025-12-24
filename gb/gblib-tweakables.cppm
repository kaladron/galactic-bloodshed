// SPDX-License-Identifier: Apache-2.0

export module gblib:tweakables;

import :types;

// Game dependencies.  These will likely change for each game.
export constexpr const char* DEADLINE = "June 22 23:00 GMT";
export constexpr const char* GAME = "The Return of Galactic Bloodshed II";
export constexpr const char* GB_VERSION = "Standard GB 5.0";
export constexpr const char* LOCATION = "courses.cstudies.ubc.ca 2010";
export constexpr const char* MODERATOR = "Chris Brougham";
export constexpr const char* PLAYERS = "25";
export constexpr const char* STARS = "100";
export constexpr const char* STARTS = "June 22 23:30 GMT";
export constexpr const char* TO = "chris@courses.cstudies.ubc.ca";
export constexpr const char* UPDATE_SCH =
    "1st 10 updates hourly. Thereafter every 30 hours.";
export constexpr const char* OTHER_STUFF =
    "\nRacegen points are increased to 1400. Moderator will play (Zargoons).\n";

// Racegen options.
export constexpr int STARTING_POINTS = 1400;
export constexpr int MIN_PASSWORD_LENGTH = 4;
export constexpr int DEFAULT_MESO_IQ_LIMIT = 180;

export constexpr const char* GB_HOST =
    "solana.mps.ohio-state.edu";      // change this for your machine
export constexpr int GB_PORT = 2010;  // change this for your port selection

export constexpr int COMMAND_TIME_MSEC =
    250;  // time slice length in milliseconds
export constexpr int COMMANDS_PER_TIME =
    1;  // commands per time slice after burst
export constexpr int COMMAND_BURST_SIZE =
    250;  // commands allowed per user in a burst
export constexpr int DISCONNECT_TIME = 7200;  // maximum idle time

export constexpr const char* WELCOME_FILE = "welcome.txt";
export constexpr const char* HELP_FILE = HELPDIR "help.md";
export constexpr const char* LEAVE_MESSAGE =
    "\n*** Thank you for playing Galactic Bloodshed ***\n";

export constexpr bool EXTERNAL_TRIGGER =
    false;  // if you wish to allow the below passwords to
            // trigger updates and movement segments
export constexpr const char* UPDATE_PASSWORD = "put_your_update_password_here";
export constexpr const char* SEGMENT_PASSWORD =
    "put_your_segment_password_here";

using namespace std::chrono_literals;
export constexpr auto DEFAULT_UPDATE_TIME = 30min;  // update time (minutes!)
export constexpr auto DEFAULT_RANDOM_UPDATE_RANGE = 0min;  // again, in minutes.
export constexpr auto DEFAULT_RANDOM_SEGMENT_RANGE =
    0min;  // again, in minutes.

/**
 * @brief If MOVES_PER_UPDATE is set to 1, there will be no movement segments
 * between updates; the move is counted as part of the update.
 * Set this to something higher to have evenly spaced movement segments.
 */
export constexpr int MOVES_PER_UPDATE = 3;

export constexpr int LOGIN_NAME_SIZE = 64;

export constexpr int COMMANDSIZE = 42;
export constexpr int MAXARGS = 256;

export constexpr int MAX_X = 45;  // top range for planet
export constexpr int MAX_Y = 19;
export constexpr double RATIOXY =
    3.70;  // map ratio between x and y. ranges of map sizes (x usually )

export constexpr int MAX_SECT_POPN = 32767;

export constexpr int TOXMAX = 20;  // max a toxwc can hold

export constexpr bool VICTORY = false;       // Use victory conditions
export constexpr bool SHOW_COWARDS = false;  // Show number of invisible players
export constexpr bool POD_TERRAFORM =
    false;                              // Pods will terraform infected sectors
export constexpr bool VOTING = true;    // Allow player voting
export constexpr bool DISSOLVE = true;  // Allow players to dissolve
export constexpr bool DEFENSE = true;   // Allow planetary guns
export constexpr bool MARKET = true;    // Enable the market

export constexpr int VICTORY_PERCENT = 10;
export constexpr int VICTORY_UPDATES = 5;

// Shipping routes - DON'T change this unless you know what you are doing
export constexpr int MAX_ROUTES = 4;

// Number of AP's to add to each player in each system.
export constexpr ap_t LIMIT_APs = 255;  // max # of APs you can have

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
export constexpr ap_t EARTH_POINTS_LOW = 5;
export constexpr ap_t EARTH_POINTS_HIGH = 8;
export constexpr ap_t MARS_POINTS_LOW = 2;
export constexpr ap_t MARS_POINTS_HIGH = 3;
export constexpr ap_t ICEBALL_POINTS_LOW = 2;
export constexpr ap_t ICEBALL_POINTS_HIGH = 3;
export constexpr ap_t GASGIANT_POINTS_LOW = 8;
export constexpr ap_t GASGIANT_POINTS_HIGH = 20;
export constexpr ap_t WATER_POINTS_LOW = 2;
export constexpr ap_t WATER_POINTS_HIGH = 3;
export constexpr ap_t FOREST_POINTS_LOW = 2;
export constexpr ap_t FOREST_POINTS_HIGH = 3;
export constexpr ap_t DESERT_POINTS_LOW = 2;
export constexpr ap_t DESERT_POINTS_HIGH = 3;

export constexpr int MAXPLAYERS = 64;
export constexpr int NUM_DISCOVERIES = 80;

export constexpr int NAMESIZE = 18;
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

export constexpr double MERCHANT_LENGTH = 200000.0;
export constexpr double INCOME_FACTOR = 0.002;
export constexpr int INSURG_FACTOR = 1;
export constexpr double UP_BID = 0.10;

export constexpr double GUN_COST = 1.00;
export constexpr double CREW_COST = 0.05;
export constexpr double CARGO_COST = 0.05;
export constexpr double FUEL_COST = 0.05;
export constexpr double AMMO_COST = 0.05;
export constexpr double SPEED_COST = 0.50;
export constexpr double CEW_COST = 0.003;
export constexpr double ARMOR_COST = 3.50;
export constexpr double HANGER_COST = 0.50;

export constexpr double AFV_FUEL_COST = 1.0;

export constexpr double MECH_ATTACK = 3.0;

export constexpr double SPORE_SUCCESS_RATE = 25;

export constexpr char CLIENT_CHAR = '|';

export constexpr int VICT_SECT = 1000;
export constexpr int VICT_SHIP = 333;
export constexpr double VICT_TECH = .10;
export constexpr int VICT_MORALE = 200;
export constexpr int VICT_RES = 100;
export constexpr int VICT_FUEL = 15;
export constexpr int VICT_MONEY = 5;
export constexpr int VICT_DIVISOR = 10000;

export constexpr double STRIKE_DISTANCE_FACTOR = 5.5;
export constexpr double COMPLEXITY_FACTOR =
    10.0;  // determines steepness of design complexity function

export constexpr char REPEAT_CHARACTER =
    ' ';  // this character makes the previous command repeat
export constexpr int POD_THRESHOLD = 18;
export constexpr int POD_DECAY = 4;
export constexpr double AP_FACTOR =
    50.0;  // how planet size affects the rate of atmosphere processing
export constexpr int DISSIPATE = 80;  // updates to dissipate dust and gases

export constexpr int MOVE_FACTOR = 1;

export constexpr int TELEG_MAX_AUTO =
    7;  // when changing, alter field in plinfo
export constexpr char TELEG_DELIM = '~';

export constexpr const char* CUTE_MESSAGE = "\nThe Galactic News\n\n";

// Planet type symbols and names
export constexpr std::array<const char, 8> Psymbol = {'@', 'o', 'O', '#',
                                                      '~', '.', ')', '-'};

export constexpr std::array<const char*, 8> Planet_types = {
    "Class M", "Asteroid",  "Airless", "Iceball",
    "Jovian",  "Waterball", "Forest",  "Desert"};

// Sector type names and symbols
export constexpr std::array<const char*, 9> Desnames = {
    "ocean",  "land",   "mountainous", "gaseous", "ice",
    "forest", "desert", "plated",      "wasted"};

export constexpr std::array<const char, 9> Dessymbols = {
    CHAR_SEA,    CHAR_LAND,   CHAR_MOUNT,  CHAR_GAS,   CHAR_ICE,
    CHAR_FOREST, CHAR_DESERT, CHAR_PLATED, CHAR_WASTED};

// Natural defenses for each sector type (maps to SectorType)
export constexpr std::array<int, 9> Defensedata = {1, 1, 3, 2, 2, 3, 2, 4, 0};
