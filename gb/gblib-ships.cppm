// SPDX-License-Identifier: Apache-2.0

export module gblib:ships;

import std;

import :gameobj;
import :planet;
import :sector;

export enum guntype_t { GTYPE_NONE, GTYPE_LIGHT, GTYPE_MEDIUM, GTYPE_HEAVY };

export inline constexpr int PRIMARY = 1;
export inline constexpr int SECONDARY = 2;

export enum ShipType : int {
  STYPE_POD,
  STYPE_SHUTTLE,
  STYPE_CARRIER,
  STYPE_DREADNT,
  STYPE_BATTLE,
  STYPE_INTCPT,
  STYPE_CRUISER,
  STYPE_DESTROYER,
  STYPE_FIGHTER,
  STYPE_EXPLORER,
  STYPE_HABITAT,
  STYPE_STATION,
  STYPE_OAP,
  STYPE_CARGO,
  STYPE_TANKER,
  STYPE_GOD,
  STYPE_MINE,
  STYPE_MIRROR,
  OTYPE_STELE,
  OTYPE_GTELE,
  OTYPE_TRACT,
  OTYPE_AP,
  OTYPE_CANIST,
  OTYPE_GREEN,
  OTYPE_VN,
  OTYPE_BERS,
  OTYPE_GOV,
  OTYPE_OMCL,
  OTYPE_TOXWC,
  OTYPE_PROBE,
  OTYPE_GR,
  OTYPE_FACTORY,
  OTYPE_TERRA,
  OTYPE_BERSCTLC,
  OTYPE_AUTOFAC,
  OTYPE_TRANSDEV,
  STYPE_MISSILE,
  OTYPE_PLANDEF,
  OTYPE_QUARRY,
  OTYPE_PLOW,
  OTYPE_DOME,
  OTYPE_WPLANT,
  OTYPE_PORT,
  OTYPE_ABM,
  OTYPE_AFV,
  OTYPE_BUNKER,
  STYPE_LANDER,
};

export enum abil_t {
  ABIL_TECH,
  ABIL_CARGO,
  ABIL_HANGER,
  ABIL_DESTCAP,
  ABIL_GUNS,
  ABIL_PRIMARY,
  ABIL_SECONDARY,
  ABIL_FUELCAP,
  ABIL_MAXCREW,
  ABIL_ARMOR,
  ABIL_COST,
  ABIL_MOUNT,
  ABIL_JUMP,
  ABIL_CANLAND,
  ABIL_HASSWITCH,
  ABIL_SPEED,
  ABIL_DAMAGE,
  ABIL_BUILD,
  ABIL_CONSTRUCT,
  ABIL_MOD,
  ABIL_LASER,
  ABIL_CEW,
  ABIL_CLOAK,
  ABIL_GOD /* only diety can build these objects */,
  ABIL_PROGRAMMED,
  ABIL_PORT,
  ABIL_REPAIR,
  ABIL_MAINTAIN
};

export inline constexpr int NUMSTYPES = (ShipType::STYPE_LANDER + 1);
export inline constexpr int NUMABILS = (ABIL_MAINTAIN + 1);

export inline constexpr int SHIP_NAMESIZE = 18;

// Special ship function data structures (converted from union members)
export struct AimedAtData {
  shipnum_t shipno; /* aimed at what ship */
  starnum_t snum;   /* aimed at what star */
  char intensity;   /* intensity of aiming */
  planetnum_t pnum; /* aimed at what planet */
  ScopeLevel level; /* aimed at what level */
};

export struct MindData {
  unsigned char progenitor; /* the originator of the strain */
  unsigned char target;     /* who to kill (for Berserkers) */
  unsigned char generation;
  unsigned char busy;     /* currently occupied */
  unsigned char tampered; /* recently tampered with? */
  unsigned char who_killed;
};

export struct PodData {
  unsigned char decay;
  unsigned char temperature;
};

export struct TimerData {
  unsigned char count;
};

export struct ImpactData {
  unsigned char x;
  unsigned char y;
  unsigned char scatter;
};

export struct TriggerData {
  unsigned short radius;
};

export struct TerraformData {
  unsigned char index;
};

export struct TransportData {
  unsigned short target;
};

export struct WasteData {
  unsigned char toxic;
};

// Variant type for special ship functions
export using SpecialData = std::variant<
  AimedAtData,    // Space Mirror
  MindData,       /* VNs and berserkers */
  PodData,        /* spore pods */
  TimerData,      /* dust canisters, greenhouse gases */
  ImpactData,     /* missiles */
  TriggerData,    /* mines */
  TerraformData,  /* terraformers */
  TransportData,  /* AVPM */
  WasteData       /* toxic waste containers */
>;

export class Ship {
 public:
  shipnum_t number;               ///< ship knows its own number
  player_t owner;                 ///< owner of ship
  governor_t governor;            ///< subordinate that controls the ship
  std::string name;               ///< name of ship (optional)
  std::string shipclass;          ///< shipclass of ship - designated by player

  unsigned char race; /* race type - used when you gain alien
                         ships during revolts and whatnot - usually
                         equal to owner */
  double xpos;
  double ypos;
  double fuel;
  double mass;
  unsigned char land_x, land_y;

  shipnum_t destshipno; /* destination ship # */
  shipnum_t nextship;   /* next ship in linked list */
  shipnum_t ships;      /* ships landed on it */

  unsigned char armor;
  unsigned short size;

  unsigned short max_crew;
  resource_t max_resource;
  unsigned short max_destruct;
  unsigned short max_fuel;
  unsigned short max_speed;
  ShipType build_type;  ///< for factories - type of ship it makes
  unsigned short build_cost;

  double base_mass;
  double tech;       /* engineering technology rating */
  double complexity; /* complexity rating */

  unsigned short destruct; /* stuff it's carrying */
  resource_t resource;
  population_t popn;   /* crew */
  population_t troops; /* marines */
  unsigned short crystals;

  /* special ship functions - now using std::variant for type safety */
  SpecialData special;

  short who_killed; /* who killed the ship */

  struct {
    unsigned char on;       /* toggles navigate mode */
    unsigned char speed;    /* speed for navigate command */
    unsigned short turns;   /* number turns left in maneuver */
    unsigned short bearing; /* course */
  } navigate;

  struct {
    double maxrng;        /* maximum range for autoshoot */
    unsigned char on;     /* toggle on/off */
    unsigned char planet; /* planet defender */
    unsigned char self;   /* retaliate if attacked */
    unsigned char evade;  /* evasive action */
    shipnum_t ship;       /* ship it is protecting */
  } protect;

  /* special systems */
  unsigned char mount; /* has a crystal mount */
  struct {
    unsigned char charge;
    unsigned char ready;
    unsigned char on;
    unsigned char has;
  } hyper_drive;
  unsigned char cew;        /* CEW strength */
  unsigned short cew_range; /* CEW (confined-energy-weapon) range */
  unsigned char cloak;      /* has cloaking device */
  unsigned char laser;      /* has a laser */
  unsigned char focus;      /* focused laser mode */
  unsigned char fire_laser; /* retaliation strength for lasers */

  starnum_t storbits;     /* what star # orbits */
  starnum_t deststar;     /* destination star */
  planetnum_t destpnum;   /* destination planet */
  planetnum_t pnumorbits; /* # of planet if orbiting */
  ScopeLevel whatdest;    /* where going */
  ScopeLevel whatorbits;  /* where orbited */

  unsigned char damage; /* amt of damage */
  int rad;              /* radiation level */
  unsigned char retaliate;
  unsigned short target;

  ShipType type;       /* what type ship is */
  unsigned char speed; /* what speed to travel at 0-9 */

  unsigned char active; /* tells whether the ship is active */
  unsigned char alive;  /* ship is alive */
  unsigned char mode;
  unsigned char bombard;  /* bombard planet we're orbiting */
  unsigned char mounted;  /* has a crystal mounted */
  unsigned char cloaked;  /* is cloaked ship */
  unsigned char sheep;    /* is under influence of mind control */
  unsigned char docked;   /* is landed on a planet or docked */
  unsigned char notified; /* has been notified of something */
  unsigned char examined; /* has been examined */
  unsigned char on;       /* on or off */

  unsigned char merchant; /* this contains the route number */
  unsigned char guns;     /* current gun system which is active */
  unsigned long primary;  /* describe primary gun system */
  guntype_t primtype;
  unsigned long secondary; /* describe secondary guns */
  guntype_t sectype;

  unsigned short hanger;     /* amount of hanger space used */
  unsigned short max_hanger; /* total hanger space */
};

export class Shiplist {
 public:
  Shiplist(shipnum_t a) : first(a) {}

  class Iterator {
   public:
    Iterator(shipnum_t a);
    auto& operator*() { return elem; }
    Iterator& operator++();
    bool operator!=(const Iterator& rhs) {
      return elem.number != rhs.elem.number;
    }

   private:
    Ship elem{};
  };

  auto begin() { return Shiplist::Iterator(first); }
  auto end() { return Shiplist::Iterator(0); }

 private:
  shipnum_t first;
};

/* can takeoff & land, is mobile, etc. */
export unsigned short speed_rating(const Ship& s);

export bool has_switch(const Ship& d);

/* can bombard planets */
export bool can_bombard(const Ship& s);

/* can navigate */
export bool can_navigate(const Ship& s);

/* can aim at things. */
export bool can_aim(const Ship& s);

/* macros to get ship stats */
export unsigned long armor(const Ship& s);
export long guns(const Ship& s);
export population_t max_crew(const Ship& s);
export population_t max_mil(const Ship& s);
export long max_resource(const Ship& s);
export int max_crystals(const Ship& s);
export long max_fuel(const Ship& s);
export long max_destruct(const Ship& s);
export long max_speed(const Ship& s);
export long shipcost(const Ship& s);
export double mass(const Ship& s);
export long shipsight(const Ship& s);
export long retaliate(const Ship& s);
export int size(const Ship& s);
export int shipbody(const Ship& s);
export long hanger(const Ship& s);
export long repair(const Ship& s);
export int getdefense(const Ship&);
export bool landed(const Ship&);
export bool laser_on(const Ship&);
export void capture_stuff(const Ship&, GameObj&);
export std::string ship_to_string(const Ship&);
export double cost(const Ship&);
export double getmass(const Ship&);
export unsigned int ship_size(const Ship&);
export double complexity(const Ship&);
export bool testship(const Ship&, GameObj&);
export void kill_ship(player_t, Ship*);
export int docked(const Ship&);
export int overloaded(const Ship&);
export std::tuple<bool, int> crash(const Ship& s, const double fuel) noexcept;
export void do_VN(Ship&);
export void planet_doVN(Ship&, Planet&, SectorMap&);
export void use_fuel(Ship&, double);
export void use_destruct(Ship&, int);
export void use_resource(Ship&, int);
export void rcv_fuel(Ship&, double);
export void rcv_resource(Ship&, int);
export void rcv_destruct(Ship&, int);
export void rcv_popn(Ship&, int, double);
export void rcv_troops(Ship&, int, double);
export std::string prin_ship_orbits(const Ship&);
export std::string prin_ship_dest(const Ship&);
export void moveship(Ship& ship, int x, int y, int z);
export void msg_OOF(const Ship& ship);
export bool followable(const Ship& ship, Ship& target);

export shipnum_t Num_ships;
export int ShipVector[NUMSTYPES];

export Ship** ships;

export std::string dispshiploc_brief(const Ship&);
export std::string dispshiploc(const Ship&);

export const char Shipltrs[] = {
    'p', 's', 'X', 'D', 'B', 'I', 'C', 'd',  'f', 'e', 'H', 'S',
    'O', 'c', 't', '!', '+', 'M', '=', '\\', '-', 'a', 'g', 'h',
    'v', 'V', '@', 'l', 'w', ':', 'G', 'F',  'T', ';', 'Z', '[',
    '^', 'P', 'q', 'K', 'Y', 'W', 'J', '&',  'R', 'b', 'L'};

// table for [ABIL_BUILD]. (bd). sum the numbers to get the correct value.
//      1 To allow it to be built on a planet.
//      2 For building by warships (d, B, C,..). Currently only for Space Probe.
//	  Mines used to be this way too. Built in hanger of building ship.
//      4 For building by Shuttles, Cargo ship, Habitats, etc.
//        Also forces construction on the outside of the ship. Not in hanger.
//      8 For building in Factories. Built on planet, or in hanger of carrying
//        ship (Habitat).
//     16 For building in Habitats. Used by Pods for instance. Also used by
//        Factories. Built inside Habitat. */

// table for [ABIL_CONSTRUCT]. (cn). sum the numbers to get the correct value.
//      1 To allow it to build like a planet.
//      2 For building like warships (d, B, C,..).
//      4 For building like Shuttles, Cargo ship, Habitats, etc.
//      8 For building like Factories.
//     16 For building like Habitats. */

// Changes here to use the new build routine using above tables.  Maarten
// Also changed:
//   - Pods, Factories, Weapons Plants, Terraforming Devices,
//     Orbital Mind Control Lasers and Government Centers can
//     be built inside Habitats.
//   - Probes, and other type 2 ships (currently none), are now built inside
//     ships, requiring hanger space. This gives more incentive to keep some
//     hanger space in the big warships.
//   - The big space stations (Habitats, Stations, and Orbital Assault
//     Platforms) can now build Probes as well.

//   - Habitats and Stations had their ability to use a crystal mount removed.
//     Since they cannot use it in any way, it was rather useless. It only
//     confused the required resources to build the ship, though this has been
//     taken care of too.
//   - Orbital Mind Control Lasers having 10 guns of caliber 0 seemed strange.
//     Now 0 guns. Also removed the 100 destruct carrying capacity. Added 25
//     cargo space so it can repair itself. */

export const long Shipdata[NUMSTYPES][NUMABILS] = {
    /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp
       dm  bd   cn mod las cew clk god prg port rep pay */
    /*SPd*/ {0, 0, 0, 0, 0, 0, 0, 20, 1, 0, 1, 0, 0, 1,
             0, 2, 0, 1, 0, 0, 0, 0,  0, 0, 1, 0, 1, 0},
    /*Shu*/ {10, 25, 2, 2, 1, 1, 0, 20, 10, 0, 2, 0, 0, 1,
             0,  4,  0, 8, 4, 1, 0, 0,  0,  0, 1, 0, 0, 1},
    /*Car*/ {250, 600, 200, 800, 30, 3, 2, 1000, 30, 5, 30, 1, 1, 0,
             0,   4,   50,  20,  2,  1, 1, 1,    0,  0, 1,  0, 1, 1},
    /*Drn*/ {300, 500, 10, 500, 60, 3, 2, 500, 60, 10, 40, 1, 1, 1,
             0,   6,   50, 8,   2,  1, 1, 1,   0,  0,  1,  0, 0, 1},
    /*BB */ {200, 235, 10, 400, 30, 3, 2, 200, 30, 7, 20, 1, 1, 1,
             0,   6,   50, 8,   2,  1, 1, 1,   0,  0, 1,  0, 0, 1},
    /*Int*/ {150, 110, 5,  120, 20, 2, 2, 200, 20, 3, 15, 1, 1, 1,
             0,   6,   50, 8,   2,  1, 1, 1,   0,  0, 1,  0, 0, 1},
    /*CA */ {150, 165, 5,  300, 20, 3, 2, 120, 20, 5, 10, 1, 1, 1,
             0,   6,   50, 8,   2,  1, 1, 1,   0,  0, 1,  0, 0, 1},
    /*DD */ {100, 110, 5,  120, 15, 2, 2, 80, 15, 3, 5, 1, 1, 1,
             0,   6,   50, 8,   2,  1, 1, 1,  0,  0, 1, 0, 0, 1},
    /*FF */ {100, 0, 0, 40, 20, 2, 1, 10, 1, 2, 1, 1, 1, 1,
             0,   9, 0, 8,  2,  1, 1, 1,  0, 0, 1, 0, 1, 1},
    /*Exp*/ {40, 10, 0, 15, 5, 2, 0, 35, 5, 1, 2, 1, 1, 1,
             0,  6,  0, 8,  0, 1, 1, 0,  0, 0, 1, 0, 0, 1},
    /*Hab*/ {100, 5000, 10, 500, 20, 2, 1, 2000, 2000, 3, 50, 0, 0, 0,
             1,   4,    75, 20,  18, 1, 0, 0,    0,    0, 1,  1, 1, 1},
    /*Stn*/ {100, 5000, 10, 250, 20, 2, 0, 2000, 50, 1, 10, 0, 0, 0,
             0,   4,    75, 20,  6,  1, 0, 0,    0,  0, 1,  1, 1, 1},
    /*OSP*/ {200, 1400, 20, 1000, 50, 3, 2, 2000, 200, 5, 40, 1, 1, 0,
             0,   4,    75, 20,   6,  1, 1, 1,    0,   0, 1,  0, 1, 1},
    /*Crg*/ {100, 1000, 5, 1000, 10, 1, 0, 1000, 100, 2, 10, 1, 1, 1,
             0,   4,    0, 8,    4,  1, 0, 0,    0,   0, 1,  0, 0, 1},
    /*Tnk*/ {100, 200, 5, 200, 10, 1, 0, 5000, 10, 2, 10, 1, 1, 1,
             0,   4,   0, 8,   2,  1, 0, 0,    0,  0, 1,  0, 0, 1},
    /*GOD*/ {9999, 20000, 1000, 20000, 1000, 3, 3, 20000, 1000, 100,
             10,   1,     1,    1,     0,    9, 0, 1,     6,    1,
             1,    1,     0,    1,     1,    1, 1, 0},
    /*SMn*/ {50, 0, 0, 25, 0, 0, 0, 20, 0, 1, 30, 0, 0, 1,
             1,  2, 0, 8,  0, 1, 0, 0,  0, 0, 1,  0, 0, 0},
    /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp
       dm  bd  cn mod las cew clk god prg port*/
    /*mir*/ {100, 200, 0,  10, 1, 1, 0, 20, 5, 0, 100, 0, 0, 0,
             0,   2,   75, 20, 0, 0, 0, 0,  0, 0, 1,   0, 1, 1},
    /*Stc*/ {50, 0, 0, 0, 0, 0, 0, 20, 2, 0, 20, 1, 1, 1,
             0,  4, 0, 8, 0, 1, 0, 0,  0, 0, 1,  0, 0, 1},
    /*Tsc*/ {5, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 1,
             0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0},
    /*T-R*/ {200, 0, 0, 0, 0, 0, 0, 1000, 5, 0, 20, 1, 0, 1,
             1,   2, 0, 1, 0, 0, 0, 0,    0, 0, 0,  0, 1, 0},
    /*APr*/ {80, 0, 0, 0, 0, 0, 0, 200, 10, 1, 20, 0, 0, 1,
             1,  0, 0, 1, 0, 0, 0, 0,   0,  0, 1,  0, 1, 0},
    /* CD*/ {40, 0, 0, 0, 0, 0, 0, 1, 0, 0, 10, 0, 0, 1,
             1,  1, 0, 1, 0, 0, 0, 0, 0, 0, 1,  0, 1, 0},
    /*Grn*/ {40, 0, 0, 0, 0, 0, 0, 1, 0, 0, 10, 0, 0, 1,
             0,  1, 0, 1, 0, 0, 0, 0, 0, 0, 1,  0, 1, 0},
    /*VN */ {80, 20, 0, 0, 0, 0, 0, 50, 0, 1, 100, 0, 0, 1,
             0,  4,  0, 1, 0, 0, 0, 0,  0, 0, 1,   0, 0, 0},
    /*Bers*/ {999, 50, 0, 500, 40, 3, 2, 1000, 0, 15, 100, 0, 0, 1,
              1,   6,  0, 1,   0,  0, 0, 0,    0, 1,  1,   0, 1, 0},
    /*Gov*/ {0, 500, 0,  100, 10, 1, 0, 1000, 10, 20, 500, 0, 0, 1,
             0, 0,   75, 17,  0,  0, 0, 0,    0,  0,  1,   1, 1, 0},
    /*OMCL*/ {350, 25, 0, 0,  0, 0, 0, 100, 2, 1, 50, 0, 0, 1,
              1,   4,  0, 17, 0, 0, 0, 0,   0, 0, 0,  0, 0, 0},
    /*TWC*/ {0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 5, 0, 0, 1,
             0, 4, 0, 1, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0},
    /*Prb*/ {150, 0, 0, 0,  0, 0, 0, 20, 0, 0, 10, 0, 0, 1,
             0,   9, 0, 19, 0, 0, 0, 0,  0, 0, 1,  0, 0, 0},
    /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp
       dm  bd  cn mod las cew clk god prg port*/
    /*GRL */ {100, 50, 0,  120, 20, 1, 0, 0, 40, 3, 30, 0, 0, 1,
              1,   0,  75, 1,   0,  1, 0, 1, 0,  0, 1,  0, 1, 1},
    /*Fac*/ {0, 50, 0,  0,  0, 0, 0, 0, 20, 0, 20, 0, 0, 1,
             1, 0,  75, 17, 8, 0, 0, 0, 0,  0, 1,  0, 1, 1},
    /*TFD*/ {50, 40, 5, 0,  0, 0, 0, 200, 20, 1, 20, 1, 1, 1,
             1,  4,  0, 17, 0, 1, 0, 0,   0,  0, 1,  0, 1, 1},
    /*BCC*/ {9999, 200, 0, 50, 0, 3, 0, 0, 0, 10, 3, 0, 0, 1,
             1,    0,   0, 1,  0, 0, 0, 0, 0, 1,  0, 0, 1, 0},
    /*BAf*/ {9999, 1000, 0, 1000, 0, 0, 0, 1000, 0, 10, 8, 0, 0, 1,
             1,    0,    0, 1,    0, 0, 0, 0,    0, 1,  0, 0, 1, 0},
    /*TD */ {200, 1000, 0,  1000, 0, 0, 0, 1000, 100, 0, 300, 0, 0, 1,
             1,   0,    50, 1,    0, 0, 0, 0,    0,   0, 1,   0, 1, 0},
    /*Mis*/ {50, 0, 0, 10, 0, 0, 0, 5, 0, 0, 5, 0, 0, 0,
             1,  6, 0, 8,  0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    /*PDN*/ {200, 50, 0,  500, 20, 3, 0, 0, 50, 10, 100, 0, 0, 1,
             1,   0,  75, 1,   0,  1, 0, 0, 0,  0,  1,   0, 1, 1},
    /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp
       dm  bd  cn mod las cew clk god prg port*/
    /*Qua*/ {0, 0, 0, 0, 0, 0, 0, 200, 50, 1, 10, 0, 0, 1,
             1, 0, 0, 1, 0, 1, 0, 0,   0,  0, 1,  0, 1, 1},
    /*Plo*/ {5, 0, 0, 0, 0, 0, 0, 200, 10, 1, 10, 0, 0, 1,
             1, 0, 0, 1, 0, 1, 0, 0,   0,  0, 1,  0, 0, 0},
    /*Dom*/ {10, 100, 0, 0, 0, 0, 0, 0, 20, 1, 10, 0, 0, 1,
             1,  0,   0, 1, 0, 1, 0, 0, 0,  0, 1,  0, 1, 0},
    /*Wea*/ {0, 500, 0,  0,  0, 0, 0, 500, 20, 5, 20, 0, 0, 1,
             0, 0,   75, 17, 0, 0, 0, 0,   0,  0, 1,  0, 1, 0},
    /*Port*/ {0, 0, 0,  0, 0, 0, 0, 0, 100, 3, 50, 0, 0, 1,
              0, 0, 75, 1, 0, 1, 0, 0, 0,   0, 1,  1, 1, 1},
    /*ABM*/ {100, 5, 0,  50, 5, 1, 0, 0, 5, 5, 50, 0, 0, 1,
             1,   0, 50, 1,  0, 1, 0, 0, 0, 0, 1,  0, 1, 1},
    /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp
       dm  bd  cn mod las cew clk god prg port*/
    /*AFV*/ {50, 5, 0, 20, 2, 1, 0, 20, 1, 2, 20, 0, 0, 0,
             0,  0, 0, 8,  0, 1, 0, 0,  0, 0, 1,  0, 1, 1},
    /*Bun*/ {10, 100, 20, 100, 0, 0, 0, 100, 100, 15, 100, 0, 0, 0,
             0,  0,   50, 1,   0, 1, 0, 0,   0,   0,  1,   0, 1, 1},
    /*Lnd*/ {150, 100, 10, 200, 10, 3, 0, 100, 500, 7, 50, 1, 1, 1,
             0,   2,   50, 8,   0,  1, 0, 0,   0,   0, 1,  0, 1, 1}};

export const char* Shipnames[NUMSTYPES] = {"Spore pod",
                                           "Shuttle",
                                           "Carrier",
                                           "Dreadnaught",
                                           "Battleship",
                                           "Interceptor",
                                           "Cruiser",
                                           "Destroyer",
                                           "Fighter Group",
                                           "Explorer",
                                           "Habitat",
                                           "Station",
                                           "Ob Asst Pltfrm",
                                           "Cargo Ship",
                                           "Tanker",
                                           "GODSHIP",
                                           "Space Mine",
                                           "Space Mirror",
                                           "Space Telescope",
                                           "Ground Telescope",
                                           "* T-R beam",
                                           "Atmosph Processor",
                                           "Dust Canister",
                                           "Greenhouse Gases",
                                           "V.Neumann Machine",
                                           "Berserker",
                                           "Govrnmnt. Center",
                                           "Mind Control Lsr",
                                           "Tox Waste Canistr",
                                           "Space Probe",
                                           "Gamma Ray Laser",
                                           "Factory",
                                           "Terraform Device",
                                           "Bers Cntrl Center",
                                           "Bers Autofac",
                                           "AVPM Transporter",
                                           "Missile",
                                           "Planet Def Net",
                                           "Quarry",
                                           "Space Plow",
                                           "Dome",
                                           "Weapons Plant",
                                           "Space Port",
                                           "ABM Battery",
                                           "Mech",
                                           "Bunker",
                                           "Lander"};

/// Type-safe accessor for primary gun caliber from Shipdata
/// \param ship_type The ship type to query
/// \return Primary gun caliber as guntype_t
export inline guntype_t shipdata_primary(ShipType ship_type) {
  return static_cast<guntype_t>(Shipdata[ship_type][ABIL_PRIMARY]);
}

/// Type-safe accessor for secondary gun caliber from Shipdata
/// \param ship_type The ship type to query
/// \return Secondary gun caliber as guntype_t
export inline guntype_t shipdata_secondary(ShipType ship_type) {
  return static_cast<guntype_t>(Shipdata[ship_type][ABIL_SECONDARY]);
}
