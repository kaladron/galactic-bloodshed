// SPDX-License-Identifier: Apache-2.0

// Client-Server Protocol (CSP) constants and types

export module gblib:csp;

export namespace GB::csp {

// Character to be found in column 0 of a CSP output line (sent by server to
// client)
inline constexpr char CSP_CLIENT = '|';

// Identifier sent by client to server when sending a command/request
inline constexpr const char* CSP_SERVER = "CSP";

// Maximum depth of scope ie: nested ships
inline constexpr int CSPD_MAXSHIP_SCOPE = 10;
inline constexpr int CSPD_NOSHIP = 0;

// Maximum number of unique letters in star and planet names
inline constexpr int MAX_SCOPE_LTRS = 4;

// Maximum server command number
inline constexpr int CSP_MAX_SERVER_COMMAND = 2000;

inline constexpr int CSP_ZOOM = 35;

// Symbols used in CSP output
inline constexpr char CSPD_XTAL_SYMBOL = 'x';
inline constexpr char CSPD_TROOP_MINE_SYMBOL = 'X';
inline constexpr char CSPD_TROOP_ALLIED_SYMBOL = 'A';
inline constexpr char CSPD_TROOP_ENEMY_SYMBOL = 'E';
inline constexpr char CSPD_TROOP_NEUTRAL_SYMBOL = 'N';

// Server responses

// VERSION
inline constexpr int CSP_VERSION_INFO = 10;     // version XX
inline constexpr int CSP_VERSION_OPTIONS = 11;  // options set YY

// LOGIN
inline constexpr int CSP_CLIENT_ON = 30;   // client mode on
inline constexpr int CSP_CLIENT_OFF = 31;  // client mode off
inline constexpr int CSP_KNOWLEDGE = 32;

// INFO
inline constexpr int CSP_SCOPE_PROMPT = 40;

// UPDATE/SEGMENT/RESET
inline constexpr int CSP_UPDATE_START = 50;   // update started
inline constexpr int CSP_UPDATE_END = 51;     // update finished
inline constexpr int CSP_SEGMENT_START = 52;  // segment started
inline constexpr int CSP_SEGMENT_END = 53;    // segment finished
inline constexpr int CSP_RESET_START = 54;    // reset started
inline constexpr int CSP_RESET_END = 55;      // reset finished
inline constexpr int CSP_BACKUP_START = 56;   // backup started
inline constexpr int CSP_BACKUP_END = 57;     // backup finished

inline constexpr int CSP_UPDATES_SUSPENDED = 58;  // updates suspended
inline constexpr int CSP_UPDATES_RESUMED = 59;    // updates resumed

// SURVEY
inline constexpr int CSP_SURVEY_INTRO = 101;   // planet info
inline constexpr int CSP_SURVEY_SECTOR = 102;  // sector info
inline constexpr int CSP_SURVEY_END = 103;     // end of command(EOC)

// RELATION
inline constexpr int CSP_RELATION_INTRO = 201;  // race # & name
inline constexpr int CSP_RELATION_DATA = 202;   // the data
inline constexpr int CSP_RELATION_END = 203;    // end of command(EOC)

// PROFILE
// DYNAMIC = Active Knowledge Capital Morale Gun GTele STele
// DYNAMIC_OTHER = %Know Morale Gun GTele OTele SecPref
inline constexpr int CSP_PROFILE_INTRO = 301;  // header
inline constexpr int CSP_PROFILE_PERSONAL = 302;
inline constexpr int CSP_PROFILE_DYNAMIC = 303;
inline constexpr int CSP_PROFILE_DYNAMIC_OTHER = 304;
inline constexpr int CSP_PROFILE_RACE_STATS = 305;
inline constexpr int CSP_PROFILE_RACE_STATS_OTHER = 306;
inline constexpr int CSP_PROFILE_PLANET = 307;
inline constexpr int CSP_PROFILE_SECTOR = 308;
inline constexpr int CSP_PROFILE_DISCOVERY = 309;  // discoveries
inline constexpr int CSP_PROFILE_END = 310;

// WHO
inline constexpr int CSP_WHO_INTRO = 401;    // header
inline constexpr int CSP_WHO_DATA = 402;     // actual data
inline constexpr int CSP_WHO_COWARDS = 403;  // send either # of cowards
inline constexpr int CSP_WHO_END = 403;      // or WHO_END as terminator

// EXPLORE
// STAR_DATA = # Name Ex Inhab Auto Slaved Toxic Compat #Sec Depo Xtal Type
inline constexpr int CSP_EXPLORE_INTRO = 501;
inline constexpr int CSP_EXPLORE_STAR = 502;
inline constexpr int CSP_EXPLORE_STAR_ALIENS = 503;
inline constexpr int CSP_EXPLORE_STAR_DATA = 504;
inline constexpr int CSP_EXPLORE_STAR_END = 505;
inline constexpr int CSP_EXPLORE_END = 506;

// MAP
// DYNAMIC_1 = Type Sects Guns MobPoints Res Des Fuel Xtals
// DYNAMIC_2 = Mob AMob Compat Pop ^Pop ^TPop Mil Tax ATax Deposits Est Prod
inline constexpr int CSP_MAP_INTRO = 601;
inline constexpr int CSP_MAP_DYNAMIC_1 = 602;
inline constexpr int CSP_MAP_DYNAMIC_2 = 603;
inline constexpr int CSP_MAP_ALIENS = 604;
inline constexpr int CSP_MAP_DATA = 605;
inline constexpr int CSP_MAP_END = 606;

// CLIENT GENERATED COMMANDS
inline constexpr int CSP_LOGIN_COMMAND = 1101;     // login command
inline constexpr int CSP_VERSION_COMMAND = 1102;   // version command
inline constexpr int CSP_SURVEY_COMMAND = 1103;    // imap command
inline constexpr int CSP_RELATION_COMMAND = 1104;  // relation command
inline constexpr int CSP_PROFILE_COMMAND = 1105;   // profile command
inline constexpr int CSP_WHO_COMMAND = 1106;       // who command
inline constexpr int CSP_EXPLORE_COMMAND = 1107;   // exploration command
inline constexpr int CSP_MAP_COMMAND = 1108;       // map command
inline constexpr int CSP_SCOPE_COMMAND = 1110;     // request a prompt

// Dan Dickey for XGB ** NOT SUPPORTED FULLY **
inline constexpr int CSP_ORBIT_COMMAND = 1501;     // orbit command
inline constexpr int CSP_ZOOM_COMMAND = 1502;      // zoom command
inline constexpr int CSP_PLANDUMP_COMMAND = 1503;  // planet dump command
inline constexpr int CSP_SHIPDUMP_COMMAND = 1504;  // ship dump command

// Planet Dumps
inline constexpr int CSP_PLANDUMP_INTRO = 2000;       // planet name
inline constexpr int CSP_PLANDUMP_CONDITIONS = 2001;  // conditions
inline constexpr int CSP_PLANDUMP_STOCK = 2002;       // stockpiles
inline constexpr int CSP_PLANDUMP_PROD = 2003;        // production last update
inline constexpr int CSP_PLANDUMP_MISC = 2004;        // rest of stuff
inline constexpr int CSP_PLANDUMP_NOEXPL = 2005;      // planet not explored

// General usage
inline constexpr int CSP_STAR_UNEXPL = 2010;  // star is not explored

// ORBIT
inline constexpr int CSP_ORBIT_OUTPUT_INTRO = 2020;   // orbit parameters
inline constexpr int CSP_ORBIT_STAR_DATA = 2021;      // star info
inline constexpr int CSP_ORBIT_UNEXP_PL_DATA = 2022;  // unexplored planet info
inline constexpr int CSP_ORBIT_EXP_PL_DATA = 2023;    // explored planet info
inline constexpr int CSP_ORBIT_SHIP_DATA = 2024;      // ship info
inline constexpr int CSP_ORBIT_OUTPUT_END = 2025;     // end of command(EOC)

// Ship Dumps
inline constexpr int CSP_SHIPDUMP_GEN = 2030;        // General information
inline constexpr int CSP_SHIPDUMP_STOCK = 2031;      // Stock information
inline constexpr int CSP_SHIPDUMP_STATUS = 2032;     // Status information
inline constexpr int CSP_SHIPDUMP_WEAPONS = 2033;    // Weapons information
inline constexpr int CSP_SHIPDUMP_FACTORY = 2034;    // Factory information
inline constexpr int CSP_SHIPDUMP_DEST = 2035;       // Destination information
inline constexpr int CSP_SHIPDUMP_PTACT_GEN = 2036;  // General planet tactical
inline constexpr int CSP_SHIPDUMP_PTACT_PDIST =
    2037;  // distance between planets
inline constexpr int CSP_SHIPDUMP_STACT_PDIST =
    2038;  // distance between a ship
inline constexpr int CSP_SHIPDUMP_PTACT_INFO =
    2039;  // for a ship from a planet
inline constexpr int CSP_SHIPDUMP_STACT_INFO = 2040;  // for a ship from a ship
inline constexpr int CSP_SHIPDUMP_ORDERS = 2041;      // Ship orders
inline constexpr int CSP_SHIPDUMP_THRESH = 2042;      // Ship threshloading
inline constexpr int CSP_SHIPDUMP_SPECIAL = 2043;     // Ship specials
inline constexpr int CSP_SHIPDUMP_HYPER = 2044;       // Hyper drive usage
inline constexpr int CSP_SHIPDUMP_END = 2055;         // end of command (EOC)

// Error Responses
inline constexpr int CSP_ERR = 9900;                  // error
inline constexpr int CSP_ERR_TOO_MANY_ARGS = 9901;    // too many args
inline constexpr int CSP_ERR_TOO_FEW_ARGS = 9902;     // too few args
inline constexpr int CSP_ERR_UNKNOWN_COMMAND = 9903;  // unknown command
inline constexpr int CSP_ERR_NOSUCH_PLAYER = 9904;    // no such player
inline constexpr int CSP_ERR_NOSUCH_PLACE = 9905;  // no such place - scope err

// Enums

// Levels for sending the prompt used in CSP_PROMPT
enum class Location {
  Univ = 0,     // CSPD_UNIV
  Star = 1,     // CSPD_STAR
  Plan = 2,     // CSPD_PLAN
  Unknown = 99  // CSPD_LOCATION_UNKNOWN
};

// Used in CSP_PROFILE among other places
enum class RaceType {
  Unknown = 0,  // CSPD_RACE_UNKNOWN
  Morph = 1,    // CSPD_RACE_MORPH
  Normal = 2    // CSPD_RACE_NORMAL
};

// Used in CSP_PROFILE and CSP_RELATION
enum class Relation {
  Unknown = 0,  // CSPD_RELAT_UNKNOWN
  Allied = 1,   // CSPD_RELAT_ALLIED
  Neutral = 2,  // CSPD_RELAT_NEUTRAL
  War = 3       // CSPD_RELAT_WAR
};

// Used in CSP_MAP
enum class Troops {
  Unknown,  // CSPD_TROOPS_UNKNOWN
  Allied,   // CSPD_TROOPS_ALLIED
  Neutral,  // CSPD_TROOPS_NEUTRAL
  Enemy,    // CSPD_TROOPS_ENEMY
  Mine      // CSPD_TROOPS_MINE
};

// Used in CSP_PROFILE for discoveries
enum class Discoveries {
  Hyperdrive = 0,   // CSPD_HYPERDRIVE
  Crystal = 1,      // CSPD_CRYSTAL
  Laser = 2,        // CSPD_LASER
  Cew = 3,          // CSPD_CEW
  Avpm = 4,         // CSPD_AVPM
  MaxNumDiscovery,  // CSPD_MAX_NUM_DISCOVERY
  Unknown = 99      // CSPD_DISCOVERY_UNKNOWN
};

enum class PlayerType {
  Normal = 0,  // CSPD_NORMAL
  Deity = 1,   // CSPD_DIETY
  Guest = 2    // CSPD_GUEST
};

enum class SectorType {
  Sea,     // CSPD_SECTOR_SEA
  Land,    // CSPD_SECTOR_LAND
  Mount,   // CSPD_SECTOR_MOUNT
  Gas,     // CSPD_SECTOR_GAS
  Ice,     // CSPD_SECTOR_ICE
  Forest,  // CSPD_SECTOR_FOREST
  Desert,  // CSPD_SECTOR_DESERT
  Plated,  // CSPD_SECTOR_PLATED
  Wasted,  // CSPD_SECTOR_WASTED
  Unknown  // CSPD_SECTOR_UNKNOWN
};

enum class PlanetType {
  ClassM,     // CSPD_PLANET_CLASS_M
  Asteroid,   // CSPD_PLANET_ASTEROID
  Airless,    // CSPD_PLANET_AIRLESS
  Iceball,    // CSPD_PLANET_ICEBALL
  Jovian,     // CSPD_PLANET_JOVIAN
  Waterball,  // CSPD_PLANET_WATERBALL
  Forest,     // CSPD_PLANET_FOREST
  Desert      // CSPD_PLANET_DESERT
};

enum class Communication {
  Broadcast,  // CSPD_BROADCAST
  Announce,   // CSPD_ANNOUNCE
  Think,      // CSPD_THINK
  Shout,      // CSPD_SHOUT
  Emote       // CSPD_EMOTE
};

}  // namespace GB::csp
