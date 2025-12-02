// SPDX-License-Identifier: Apache-2.0

export module gblib:ships;

import std;

import :gameobj;
import :planet;
import :sector;
import :turnstats;

export enum guntype_t {
  GTYPE_NONE,
  GTYPE_LIGHT,
  GTYPE_MEDIUM,
  GTYPE_HEAVY
};

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
export using SpecialData =
    std::variant<AimedAtData,   // Space Mirror
                 MindData,      /* VNs and berserkers */
                 PodData,       /* spore pods */
                 TimerData,     /* dust canisters, greenhouse gases */
                 ImpactData,    /* missiles */
                 TriggerData,   /* mines */
                 TerraformData, /* terraformers */
                 TransportData, /* AVPM */
                 WasteData      /* toxic waste containers */
                 >;

// Named structs for nested ship data (replacing anonymous structs)
export struct NavigateData {
  unsigned char on{0};       /* toggles navigate mode */
  unsigned char speed{0};    /* speed for navigate command */
  unsigned short turns{0};   /* number turns left in maneuver */
  unsigned short bearing{0}; /* course */
};

export struct ProtectData {
  double maxrng{0.0};      /* maximum range for autoshoot */
  unsigned char on{0};     /* toggle on/off */
  unsigned char planet{0}; /* planet defender */
  unsigned char self{0};   /* retaliate if attacked */
  unsigned char evade{0};  /* evasive action */
  shipnum_t ship{0};       /* ship it is protecting */
};

export struct HyperDriveData {
  unsigned char charge{0};
  unsigned char ready{0};
  unsigned char on{0};
  unsigned char has{0};
};

// POD struct containing all Ship data fields for serialization
export struct ship_struct {
  shipnum_t number{0};     ///< ship knows its own number
  player_t owner{0};       ///< owner of ship
  governor_t governor{0};  ///< subordinate that controls the ship
  std::string name;        ///< name of ship (optional)
  std::string shipclass;   ///< shipclass of ship - designated by player

  unsigned char race{0}; /* race type - used when you gain alien
                         ships during revolts and whatnot - usually
                         equal to owner */
  double xpos{0.0};
  double ypos{0.0};
  double fuel{0.0};
  double mass{0.0};
  unsigned char land_x{0}, land_y{0};

  shipnum_t destshipno{0}; /* destination ship # */
  shipnum_t nextship{0};   /* next ship in linked list */
  shipnum_t ships{0};      /* ships landed on it */

  unsigned char armor{0};
  unsigned short size{0};

  unsigned short max_crew{0};
  resource_t max_resource{0};
  unsigned short max_destruct{0};
  unsigned short max_fuel{0};
  unsigned short max_speed{0};
  ShipType build_type{ShipType::STYPE_POD};
  unsigned short build_cost{0};

  double base_mass{0.0};
  double tech{0.0};
  double complexity{0.0};

  unsigned short destruct{0};
  resource_t resource{0};
  population_t popn{0};
  population_t troops{0};
  unsigned short crystals{0};

  SpecialData special{};

  short who_killed{0};

  NavigateData navigate;
  ProtectData protect;

  unsigned char mount{0};
  HyperDriveData hyper_drive;
  unsigned char cew{0};
  unsigned short cew_range{0};
  unsigned char cloak{0};
  unsigned char laser{0};
  unsigned char focus{0};
  unsigned char fire_laser{0};

  starnum_t storbits{0};
  starnum_t deststar{0};
  planetnum_t destpnum{0};
  planetnum_t pnumorbits{0};
  ScopeLevel whatdest{ScopeLevel::LEVEL_UNIV};
  ScopeLevel whatorbits{ScopeLevel::LEVEL_UNIV};

  unsigned char damage{0};
  int rad{0};
  unsigned char retaliate{0};
  unsigned short target{0};

  ShipType type{ShipType::STYPE_POD};
  unsigned char speed{0};

  unsigned char active{0};
  unsigned char alive{0};
  unsigned char mode{0};
  unsigned char bombard{0};
  unsigned char mounted{0};
  unsigned char cloaked{0};
  unsigned char sheep{0};
  unsigned char docked{0};
  unsigned char notified{0};
  unsigned char examined{0};
  unsigned char on{0};

  unsigned char merchant{0};
  unsigned char guns{0};
  unsigned long primary{0};
  guntype_t primtype{GTYPE_NONE};
  unsigned long secondary{0};
  guntype_t sectype{GTYPE_NONE};

  unsigned short hanger{0};
  unsigned short max_hanger{0};
};

export class Ship {
private:
  ship_struct data_;  // Private data member for encapsulation

public:
  // Constructors
  Ship() = default;
  Ship(ship_struct in) : data_(std::move(in)) {}
  ~Ship() = default;

  // Delete copy, allow move
  Ship(const Ship&) = delete;
  Ship& operator=(const Ship&) = delete;
  Ship(Ship&& other) noexcept : data_(std::move(other.data_)) {}
  Ship& operator=(Ship&& other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
    }
    return *this;
  }

  // =========================================================================
  // ACCESSOR METHODS - const and non-const pairs
  // =========================================================================

  // Ship identity
  [[nodiscard]] shipnum_t number() const {
    return data_.number;
  }
  shipnum_t& number() {
    return data_.number;
  }

  [[nodiscard]] player_t owner() const {
    return data_.owner;
  }
  player_t& owner() {
    return data_.owner;
  }

  [[nodiscard]] governor_t governor() const {
    return data_.governor;
  }
  governor_t& governor() {
    return data_.governor;
  }

  [[nodiscard]] const std::string& name() const {
    return data_.name;
  }
  std::string& name() {
    return data_.name;
  }

  [[nodiscard]] const std::string& shipclass() const {
    return data_.shipclass;
  }
  std::string& shipclass() {
    return data_.shipclass;
  }

  [[nodiscard]] unsigned char race() const {
    return data_.race;
  }
  unsigned char& race() {
    return data_.race;
  }

  // Position
  [[nodiscard]] double xpos() const {
    return data_.xpos;
  }
  double& xpos() {
    return data_.xpos;
  }

  [[nodiscard]] double ypos() const {
    return data_.ypos;
  }
  double& ypos() {
    return data_.ypos;
  }

  // Resources
  [[nodiscard]] double fuel() const {
    return data_.fuel;
  }
  double& fuel() {
    return data_.fuel;
  }

  [[nodiscard]] double mass() const {
    return data_.mass;
  }
  double& mass() {
    return data_.mass;
  }

  [[nodiscard]] unsigned char land_x() const {
    return data_.land_x;
  }
  unsigned char& land_x() {
    return data_.land_x;
  }

  [[nodiscard]] unsigned char land_y() const {
    return data_.land_y;
  }
  unsigned char& land_y() {
    return data_.land_y;
  }

  // Ship references
  [[nodiscard]] shipnum_t destshipno() const {
    return data_.destshipno;
  }
  shipnum_t& destshipno() {
    return data_.destshipno;
  }

  [[nodiscard]] shipnum_t nextship() const {
    return data_.nextship;
  }
  shipnum_t& nextship() {
    return data_.nextship;
  }

  [[nodiscard]] shipnum_t ships() const {
    return data_.ships;
  }
  shipnum_t& ships() {
    return data_.ships;
  }

  // Stats
  [[nodiscard]] unsigned char armor() const {
    return data_.armor;
  }
  unsigned char& armor() {
    return data_.armor;
  }

  [[nodiscard]] unsigned short size() const {
    return data_.size;
  }
  unsigned short& size() {
    return data_.size;
  }

  [[nodiscard]] unsigned short max_crew() const {
    return data_.max_crew;
  }
  unsigned short& max_crew() {
    return data_.max_crew;
  }

  [[nodiscard]] resource_t max_resource() const {
    return data_.max_resource;
  }
  resource_t& max_resource() {
    return data_.max_resource;
  }

  [[nodiscard]] unsigned short max_destruct() const {
    return data_.max_destruct;
  }
  unsigned short& max_destruct() {
    return data_.max_destruct;
  }

  [[nodiscard]] unsigned short max_fuel() const {
    return data_.max_fuel;
  }
  unsigned short& max_fuel() {
    return data_.max_fuel;
  }

  [[nodiscard]] unsigned short max_speed() const {
    return data_.max_speed;
  }
  unsigned short& max_speed() {
    return data_.max_speed;
  }

  // Build info
  [[nodiscard]] ShipType build_type() const {
    return data_.build_type;
  }
  ShipType& build_type() {
    return data_.build_type;
  }

  [[nodiscard]] unsigned short build_cost() const {
    return data_.build_cost;
  }
  unsigned short& build_cost() {
    return data_.build_cost;
  }

  [[nodiscard]] double base_mass() const {
    return data_.base_mass;
  }
  double& base_mass() {
    return data_.base_mass;
  }

  [[nodiscard]] double tech() const {
    return data_.tech;
  }
  double& tech() {
    return data_.tech;
  }

  [[nodiscard]] double complexity() const {
    return data_.complexity;
  }
  double& complexity() {
    return data_.complexity;
  }

  // Cargo
  [[nodiscard]] unsigned short destruct() const {
    return data_.destruct;
  }
  unsigned short& destruct() {
    return data_.destruct;
  }

  [[nodiscard]] resource_t resource() const {
    return data_.resource;
  }
  resource_t& resource() {
    return data_.resource;
  }

  [[nodiscard]] population_t popn() const {
    return data_.popn;
  }
  population_t& popn() {
    return data_.popn;
  }

  [[nodiscard]] population_t troops() const {
    return data_.troops;
  }
  population_t& troops() {
    return data_.troops;
  }

  [[nodiscard]] unsigned short crystals() const {
    return data_.crystals;
  }
  unsigned short& crystals() {
    return data_.crystals;
  }

  // Special data
  [[nodiscard]] const SpecialData& special() const {
    return data_.special;
  }
  SpecialData& special() {
    return data_.special;
  }

  [[nodiscard]] short who_killed() const {
    return data_.who_killed;
  }
  short& who_killed() {
    return data_.who_killed;
  }

  // Navigation
  [[nodiscard]] const NavigateData& navigate() const {
    return data_.navigate;
  }
  NavigateData& navigate() {
    return data_.navigate;
  }

  // Protection
  [[nodiscard]] const ProtectData& protect() const {
    return data_.protect;
  }
  ProtectData& protect() {
    return data_.protect;
  }

  // Special systems
  [[nodiscard]] unsigned char mount() const {
    return data_.mount;
  }
  unsigned char& mount() {
    return data_.mount;
  }

  [[nodiscard]] const HyperDriveData& hyper_drive() const {
    return data_.hyper_drive;
  }
  HyperDriveData& hyper_drive() {
    return data_.hyper_drive;
  }

  [[nodiscard]] unsigned char cew() const {
    return data_.cew;
  }
  unsigned char& cew() {
    return data_.cew;
  }

  [[nodiscard]] unsigned short cew_range() const {
    return data_.cew_range;
  }
  unsigned short& cew_range() {
    return data_.cew_range;
  }

  [[nodiscard]] unsigned char cloak() const {
    return data_.cloak;
  }
  unsigned char& cloak() {
    return data_.cloak;
  }

  [[nodiscard]] unsigned char laser() const {
    return data_.laser;
  }
  unsigned char& laser() {
    return data_.laser;
  }

  [[nodiscard]] unsigned char focus() const {
    return data_.focus;
  }
  unsigned char& focus() {
    return data_.focus;
  }

  [[nodiscard]] unsigned char fire_laser() const {
    return data_.fire_laser;
  }
  unsigned char& fire_laser() {
    return data_.fire_laser;
  }

  // Location
  [[nodiscard]] starnum_t storbits() const {
    return data_.storbits;
  }
  starnum_t& storbits() {
    return data_.storbits;
  }

  [[nodiscard]] starnum_t deststar() const {
    return data_.deststar;
  }
  starnum_t& deststar() {
    return data_.deststar;
  }

  [[nodiscard]] planetnum_t destpnum() const {
    return data_.destpnum;
  }
  planetnum_t& destpnum() {
    return data_.destpnum;
  }

  [[nodiscard]] planetnum_t pnumorbits() const {
    return data_.pnumorbits;
  }
  planetnum_t& pnumorbits() {
    return data_.pnumorbits;
  }

  [[nodiscard]] ScopeLevel whatdest() const {
    return data_.whatdest;
  }
  ScopeLevel& whatdest() {
    return data_.whatdest;
  }

  [[nodiscard]] ScopeLevel whatorbits() const {
    return data_.whatorbits;
  }
  ScopeLevel& whatorbits() {
    return data_.whatorbits;
  }

  // Combat
  [[nodiscard]] unsigned char damage() const {
    return data_.damage;
  }
  unsigned char& damage() {
    return data_.damage;
  }

  [[nodiscard]] int rad() const {
    return data_.rad;
  }
  int& rad() {
    return data_.rad;
  }

  [[nodiscard]] unsigned char retaliate() const {
    return data_.retaliate;
  }
  unsigned char& retaliate() {
    return data_.retaliate;
  }

  [[nodiscard]] unsigned short target() const {
    return data_.target;
  }
  unsigned short& target() {
    return data_.target;
  }

  // Type and speed
  [[nodiscard]] ShipType type() const {
    return data_.type;
  }
  ShipType& type() {
    return data_.type;
  }

  [[nodiscard]] unsigned char speed() const {
    return data_.speed;
  }
  unsigned char& speed() {
    return data_.speed;
  }

  // Status flags
  [[nodiscard]] unsigned char active() const {
    return data_.active;
  }
  unsigned char& active() {
    return data_.active;
  }

  [[nodiscard]] unsigned char alive() const {
    return data_.alive;
  }
  unsigned char& alive() {
    return data_.alive;
  }

  [[nodiscard]] unsigned char mode() const {
    return data_.mode;
  }
  unsigned char& mode() {
    return data_.mode;
  }

  [[nodiscard]] unsigned char bombard() const {
    return data_.bombard;
  }
  unsigned char& bombard() {
    return data_.bombard;
  }

  [[nodiscard]] unsigned char mounted() const {
    return data_.mounted;
  }
  unsigned char& mounted() {
    return data_.mounted;
  }

  [[nodiscard]] unsigned char cloaked() const {
    return data_.cloaked;
  }
  unsigned char& cloaked() {
    return data_.cloaked;
  }

  [[nodiscard]] unsigned char sheep() const {
    return data_.sheep;
  }
  unsigned char& sheep() {
    return data_.sheep;
  }

  [[nodiscard]] unsigned char docked() const {
    return data_.docked;
  }
  unsigned char& docked() {
    return data_.docked;
  }

  [[nodiscard]] unsigned char notified() const {
    return data_.notified;
  }
  unsigned char& notified() {
    return data_.notified;
  }

  [[nodiscard]] unsigned char examined() const {
    return data_.examined;
  }
  unsigned char& examined() {
    return data_.examined;
  }

  [[nodiscard]] unsigned char on() const {
    return data_.on;
  }
  unsigned char& on() {
    return data_.on;
  }

  // Merchant and weapons
  [[nodiscard]] unsigned char merchant() const {
    return data_.merchant;
  }
  unsigned char& merchant() {
    return data_.merchant;
  }

  [[nodiscard]] unsigned char guns() const {
    return data_.guns;
  }
  unsigned char& guns() {
    return data_.guns;
  }

  [[nodiscard]] unsigned long primary() const {
    return data_.primary;
  }
  unsigned long& primary() {
    return data_.primary;
  }

  [[nodiscard]] guntype_t primtype() const {
    return data_.primtype;
  }
  guntype_t& primtype() {
    return data_.primtype;
  }

  [[nodiscard]] unsigned long secondary() const {
    return data_.secondary;
  }
  unsigned long& secondary() {
    return data_.secondary;
  }

  [[nodiscard]] guntype_t sectype() const {
    return data_.sectype;
  }
  guntype_t& sectype() {
    return data_.sectype;
  }

  // Hanger
  [[nodiscard]] unsigned short hanger() const {
    return data_.hanger;
  }
  unsigned short& hanger() {
    return data_.hanger;
  }

  [[nodiscard]] unsigned short max_hanger() const {
    return data_.max_hanger;
  }
  unsigned short& max_hanger() {
    return data_.max_hanger;
  }

  // =========================================================================
  // SERIALIZATION SUPPORT
  // =========================================================================

  // For repository serialization - returns copy of internal struct
  [[nodiscard]] ship_struct get_struct() const {
    return data_;
  }

  // Direct access to internal struct (FOR SERIALIZATION USE ONLY)
  [[nodiscard]] const ship_struct& to_struct() const noexcept {
    return data_;
  }
  [[nodiscard]] ship_struct& to_struct() noexcept {
    return data_;
  }
};

export class Shiplist {
public:
  Shiplist(shipnum_t a) : first(a) {}

  class Iterator {
  public:
    Iterator(shipnum_t a);
    Ship& operator*() {
      return elem_ship;
    }
    Iterator& operator++();
    bool operator!=(const Iterator& rhs) {
      return elem_ship.number() != rhs.elem_ship.number();
    }

  private:
    ship_struct elem_data{};    // Store POD struct (copyable)
    Ship elem_ship{elem_data};  // Wrap in Ship for access
  };

  auto begin() {
    return Shiplist::Iterator(first);
  }
  auto end() {
    return Shiplist::Iterator(0);
  }

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
export void do_VN(Ship&, TurnStats&);
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

/// Get display character for gun caliber type
/// \param caliber Gun caliber type (GTYPE_NONE=0, GTYPE_LIGHT=1,
/// GTYPE_MEDIUM=2, GTYPE_HEAVY=3)
/// \return Character representing caliber ('L', 'M', 'H', or ' ' for none)
export constexpr char caliber_char(guntype_t caliber) {
  switch (caliber) {
    case GTYPE_LIGHT:
      return 'L';
    case GTYPE_MEDIUM:
      return 'M';
    case GTYPE_HEAVY:
      return 'H';
    case GTYPE_NONE:
    default:
      return ' ';
  }
}

/// Check if ship type appears in filter string
/// \param type Ship type to check
/// \param filter String containing ship type letters to match
/// \return True if ship type letter appears in filter string
export inline bool listed(ShipType type, std::string_view filter) {
  return std::ranges::any_of(filter,
                             [type](char c) { return Shipltrs[type] == c; });
}
