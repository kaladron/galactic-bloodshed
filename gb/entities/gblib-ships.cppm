// SPDX-License-Identifier: Apache-2.0

/// \file gblib-ships.cppm
/// \brief Module interface partition for Ship domain entities, types, and
/// stats.

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

export struct ShipExam {
  ShipType ship_type{ShipType::STYPE_POD};
  std::string name;
  std::string description;
};

// Special ship function data structures (converted from union members)
export struct AimedAtData {
  shipnum_t shipno; /* aimed at what ship */
  starnum_t snum;   /* aimed at what star */
  char intensity;   /* intensity of aiming */
  planetnum_t pnum; /* aimed at what planet */
  ScopeLevel level; /* aimed at what level */
};

/// Brain parameters for Von Neumann machines and Berserkers.
export struct MindData {
  player_t progenitor{0};       ///< Original race that created this strain
  player_t target{0};           ///< Target player to destroy (for Berserkers)
  std::uint32_t generation{0};  ///< Reproduction generation counter
  bool busy{false};      ///< Whether machine is currently occupied with a task
  bool tampered{false};  ///< Whether machine brain was reprogrammed by an alien
  player_t who_killed{0};  ///< Player who destroyed progenitor machine
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

/// Automated navigation course parameters for a ship.
export struct NavigateData {
  bool on{false};          ///< Whether navigation course mode is active
  speed_t speed{0};        ///< Dialed navigation speed throttle (0..9)
  std::uint32_t turns{0};  ///< Movement turns remaining in maneuver
  bearing_t bearing{0};    ///< Course heading in degrees (0..359)
};

/// Defensive escort and auto-retaliation parameters for a ship.
export struct ProtectData {
  double maxrng{0.0};  ///< Maximum engagement range for defense fire
  shipnum_t ship{0};   ///< Target ship number being protected
  bool on{false};      ///< Whether escort / protection mode is active
  bool planet{false};  ///< Whether assigned as a planetary defense interceptor
  bool self{false};    ///< Whether ship automatically retaliates when attacked
  bool evade{false};   ///< Whether ship executes evasive maneuvers in combat
};

/// Faster-than-light hyperdrive parameters.
export struct HyperDriveData {
  std::uint32_t charge{
      0};           ///< Charge accumulator (0..HYPER_DRIVE_READY_CHARGE)
  bool on{false};   ///< Whether hyperdrive charging / jump sequence is engaged
  bool has{false};  ///< Whether ship is equipped with a functional hyperdrive

  /// Returns whether hyperdrive is fully charged and ready for jump.
  [[nodiscard]] constexpr bool is_ready() const noexcept {
    return charge >= HYPER_DRIVE_READY_CHARGE;
  }
};

// POD struct containing all Ship data fields for serialization
export struct ship_struct {
  shipnum_t number{0};     ///< Ship's unique identification number
  player_t owner{0};       ///< Owner player ID
  governor_t governor{0};  ///< Governor controlling the ship
  std::string name;        ///< Name of ship (optional)
  std::string shipclass;   ///< Ship class designated by player

  player_t race{0};  ///< Race type (usually equal to owner, distinct after
                     ///< capture/revolt)
  double xpos{0.0};  ///< X position coordinate
  double ypos{0.0};  ///< Y position coordinate
  double fuel{0.0};  ///< Current stored fuel
  double mass{0.0};  ///< Current total mass
  Coordinates land_coords{0, 0};  ///< Planetary surface coordinates when landed

  shipnum_t destshipno{0};  ///< Destination / escorted ship number
  shipnum_t nextship{0};    ///< Next ship in fleet or sector linked list
  shipnum_t ships{0};       ///< First ship landed on or docked in this carrier

  armor_t armor{0};     ///< Armor protection rating
  ship_size_t size{0};  ///< Ship hull volume / physical size

  population_t max_crew{0};        ///< Maximum crew capacity
  resource_t max_resource{0};      ///< Maximum resource cargo capacity
  unsigned short max_destruct{0};  ///< Maximum destructive charge capacity
  unsigned short max_fuel{0};      ///< Maximum fuel tank capacity
  speed_t max_speed{0};            ///< Maximum engine impulse speed
  ShipType build_type{
      ShipType::STYPE_POD};      ///< Ship template type when constructed
  unsigned short build_cost{0};  ///< Construction cost in resources

  double base_mass{0.0};   ///< Empty hull baseline mass
  double tech{0.0};        ///< Construction technology level
  double complexity{0.0};  ///< Hull structural complexity rating

  unsigned short destruct{0};  ///< Current carried destructive charges
  resource_t resource{0};      ///< Current carried resource cargo
  population_t popn{0};        ///< Current carried colonists / crew
  population_t troops{0};      ///< Current carried military troops
  unsigned short crystals{0};  ///< Current carried warp crystal charge

  SpecialData special;  ///< Ship-type-specific payload / mode data

  player_t who_killed{0};  ///< Player ID responsible for destroying the ship

  NavigateData navigate;  ///< Standing navigational heading orders
  ProtectData protect;    ///< Escort, defense, and evasion orders

  bool mount{false};            ///< Crystal mount equipped
  HyperDriveData hyper_drive;   ///< Hyperspace jump drive systems
  weapon_power_t cew{0};        ///< Concentrated energy weapon power rating
  unsigned short cew_range{0};  ///< CEW beam operational range
  bool cloak{false};            ///< Cloaking device equipped
  bool laser{false};            ///< Combat laser weapon equipped
  bool focus{false};            ///< Laser focus mode enabled
  bool fire_laser{false};       ///< Combat laser armed for firing

  starnum_t storbits{0};      ///< Star system currently orbited
  starnum_t deststar{0};      ///< Destination star system
  planetnum_t destpnum{0};    ///< Destination planet number
  planetnum_t pnumorbits{0};  ///< Planet currently orbited
  ScopeLevel whatdest{ScopeLevel::LEVEL_UNIV};  ///< Destination scope level
  ScopeLevel whatorbits{
      ScopeLevel::LEVEL_UNIV};  ///< Current orbit / location scope level

  damage_t damage{0};           ///< Structural damage percentage (0-100)
  radiation_t rad{0};           ///< Radiation contamination level
  weapon_power_t retaliate{0};  ///< Salvo size / max power used in retaliation
  shipnum_t target{0};          ///< Current tactical weapon target ship number

  ShipType type{ShipType::STYPE_POD};  ///< Operational ship type classification
  speed_t speed{0};                    ///< Current impulse speed throttle

  bool active{false};  ///< Operational / crewed status
  bool alive{false};   ///< Ship hull intact / not destroyed
  bool mode{
      false};  ///< Warhead detonation mode (false: explosive, true: radiative)
  bool bombard{false};   ///< Planetary bombardment enabled
  bool mounted{false};   ///< Warp crystal currently mounted in jump drive
  bool cloaked{false};   ///< Cloaking device active
  bool sheep{false};     ///< Sub-light exploration automation enabled
  bool docked{false};    ///< Docked inside a carrier ship
  bool notified{false};  ///< Player notified of arrival / event
  bool examined{false};  ///< Ship surveyed / examined
  bool on{false};        ///< Factory / power generator online

  bool merchant{false};            ///< Commercial trade vessel status
  gun_count_t guns{0};             ///< Active gun battery configuration
  weapon_power_t primary{0};       ///< Primary battery weapon payload
  guntype_t primtype{GTYPE_NONE};  ///< Primary gun caliber type
  weapon_power_t secondary{0};     ///< Secondary battery weapon payload
  guntype_t sectype{GTYPE_NONE};   ///< Secondary gun caliber type

  hangar_t hanger{0};      ///< Current docked fighters / payload count
  hangar_t max_hanger{0};  ///< Maximum hangar capacity
};

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

export class Ship {
protected:
  ship_struct
      data_;  // Protected data member for encapsulation and subclass access

public:
  // Constructors
  Ship() = default;
  Ship(ship_struct in) : data_(std::move(in)) {}
  virtual ~Ship() = default;

  template <typename Derived>
  [[nodiscard]] Derived* as() noexcept;

  template <typename Derived>
  [[nodiscard]] const Derived* as() const noexcept;

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

  [[nodiscard]] player_t race() const {
    return data_.race;
  }
  player_t& race() {
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

  [[nodiscard]] Coordinates land_coords() const noexcept {
    return data_.land_coords;
  }
  void set_land_coords(const Coordinates c) noexcept {
    data_.land_coords = c;
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
  [[nodiscard]] armor_t armor() const {
    return data_.armor;
  }
  armor_t& armor() {
    return data_.armor;
  }

  [[nodiscard]] ship_size_t size() const {
    return data_.size;
  }
  ship_size_t& size() {
    return data_.size;
  }

  [[nodiscard]] population_t max_crew() const {
    return data_.max_crew;
  }
  population_t& max_crew() {
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

  [[nodiscard]] speed_t max_speed() const {
    return data_.max_speed;
  }
  speed_t& max_speed() {
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

  [[nodiscard]] player_t who_killed() const {
    return data_.who_killed;
  }
  player_t& who_killed() {
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
  [[nodiscard]] bool mount() const {
    return data_.mount;
  }
  bool& mount() {
    return data_.mount;
  }

  [[nodiscard]] const HyperDriveData& hyper_drive() const {
    return data_.hyper_drive;
  }
  HyperDriveData& hyper_drive() {
    return data_.hyper_drive;
  }

  [[nodiscard]] weapon_power_t cew() const {
    return data_.cew;
  }
  weapon_power_t& cew() {
    return data_.cew;
  }

  [[nodiscard]] unsigned short cew_range() const {
    return data_.cew_range;
  }
  unsigned short& cew_range() {
    return data_.cew_range;
  }

  [[nodiscard]] bool cloak() const {
    return data_.cloak;
  }
  bool& cloak() {
    return data_.cloak;
  }

  [[nodiscard]] bool laser() const {
    return data_.laser;
  }
  bool& laser() {
    return data_.laser;
  }

  [[nodiscard]] bool focus() const {
    return data_.focus;
  }
  bool& focus() {
    return data_.focus;
  }

  [[nodiscard]] bool fire_laser() const {
    return data_.fire_laser;
  }
  bool& fire_laser() {
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
  [[nodiscard]] damage_t damage() const {
    return data_.damage;
  }
  damage_t& damage() {
    return data_.damage;
  }

  [[nodiscard]] radiation_t rad() const {
    return data_.rad;
  }
  radiation_t& rad() {
    return data_.rad;
  }

  [[nodiscard]] weapon_power_t retaliate() const {
    return data_.retaliate;
  }
  weapon_power_t& retaliate() {
    return data_.retaliate;
  }

  [[nodiscard]] shipnum_t target() const {
    return data_.target;
  }
  shipnum_t& target() {
    return data_.target;
  }

  // Type and speed
  [[nodiscard]] ShipType type() const {
    return data_.type;
  }
  ShipType& type() {
    return data_.type;
  }

  [[nodiscard]] speed_t speed() const {
    return data_.speed;
  }
  speed_t& speed() {
    return data_.speed;
  }

  // Status flags
  [[nodiscard]] bool active() const {
    return data_.active;
  }
  bool& active() {
    return data_.active;
  }

  [[nodiscard]] bool alive() const {
    return data_.alive;
  }
  bool& alive() {
    return data_.alive;
  }

  [[nodiscard]] bool mode() const {
    return data_.mode;
  }
  bool& mode() {
    return data_.mode;
  }

  [[nodiscard]] bool bombard() const {
    return data_.bombard;
  }
  bool& bombard() {
    return data_.bombard;
  }

  [[nodiscard]] bool mounted() const {
    return data_.mounted;
  }
  bool& mounted() {
    return data_.mounted;
  }

  [[nodiscard]] bool cloaked() const {
    return data_.cloaked;
  }
  bool& cloaked() {
    return data_.cloaked;
  }

  [[nodiscard]] bool sheep() const {
    return data_.sheep;
  }
  bool& sheep() {
    return data_.sheep;
  }

  [[nodiscard]] bool docked() const {
    return data_.docked;
  }
  bool& docked() {
    return data_.docked;
  }

  [[nodiscard]] bool notified() const {
    return data_.notified;
  }
  bool& notified() {
    return data_.notified;
  }

  [[nodiscard]] bool examined() const {
    return data_.examined;
  }
  bool& examined() {
    return data_.examined;
  }

  [[nodiscard]] bool on() const {
    return data_.on;
  }
  bool& on() {
    return data_.on;
  }

  // Merchant and weapons
  [[nodiscard]] bool merchant() const {
    return data_.merchant;
  }
  bool& merchant() {
    return data_.merchant;
  }

  [[nodiscard]] gun_count_t guns() const {
    return data_.guns;
  }
  gun_count_t& guns() {
    return data_.guns;
  }

  [[nodiscard]] weapon_power_t primary() const {
    return data_.primary;
  }
  weapon_power_t& primary() {
    return data_.primary;
  }

  [[nodiscard]] guntype_t primtype() const {
    return data_.primtype;
  }
  guntype_t& primtype() {
    return data_.primtype;
  }

  [[nodiscard]] weapon_power_t secondary() const {
    return data_.secondary;
  }
  weapon_power_t& secondary() {
    return data_.secondary;
  }

  [[nodiscard]] guntype_t sectype() const {
    return data_.sectype;
  }
  guntype_t& sectype() {
    return data_.sectype;
  }

  // Hanger
  [[nodiscard]] hangar_t hanger() const {
    return data_.hanger;
  }
  hangar_t& hanger() {
    return data_.hanger;
  }

  [[nodiscard]] hangar_t max_hanger() const {
    return data_.max_hanger;
  }
  hangar_t& max_hanger() {
    return data_.max_hanger;
  }

  // =========================================================================
  // DOMAIN QUERIES & COMPUTED PROPERTIES
  // =========================================================================

  /// Whether ship is currently docked inside another ship (mothership/carrier).
  [[nodiscard]] bool is_docked() const noexcept {
    return data_.docked && data_.whatdest == ScopeLevel::LEVEL_SHIP;
  }

  /// Whether ship is currently landed on a planet surface.
  [[nodiscard]] bool is_landed() const noexcept {
    return data_.whatdest == ScopeLevel::LEVEL_PLAN && data_.docked;
  }

  /// Whether ship has an active combat laser armed and ready to fire.
  [[nodiscard]] bool is_laser_on() const noexcept {
    return data_.laser && data_.fire_laser;
  }

  /// Whether hyperspace jump drive has accumulated sufficient charge to jump.
  [[nodiscard]] bool is_hyper_drive_ready() const noexcept {
    return data_.hyper_drive.is_ready();
  }

  /// Whether ship cargo, fuel, crew, or ammo exceeds design storage limits.
  [[nodiscard]] bool is_overloaded() const noexcept {
    return (data_.resource > max_resource_capacity()) ||
           (data_.fuel > max_fuel_capacity()) ||
           (data_.popn + data_.troops > max_crew_capacity()) ||
           (data_.destruct > max_destruct_capacity());
  }

  /// Whether ship type has an operational on/off activation switch.
  [[nodiscard]] bool has_switch() const noexcept {
    return Shipdata[data_.type][ABIL_HASSWITCH] != 0;
  }

  /// Whether ship has planetary bombardment weapon capabilities.
  [[nodiscard]] bool can_bombard() const noexcept {
    return Shipdata[data_.type][ABIL_GUNS] != 0 &&
           (data_.type != ShipType::STYPE_MINE);
  }

  /// Whether ship is capable of independent orbital navigation.
  [[nodiscard]] bool can_navigate() const noexcept {
    return Shipdata[data_.type][ABIL_SPEED] > 0 &&
           data_.type != ShipType::OTYPE_TERRA &&
           data_.type != ShipType::OTYPE_VN;
  }

  /// Whether ship can be aimed at specific orbital targets (mirrors/tractors).
  [[nodiscard]] bool can_aim() const noexcept {
    return data_.type >= ShipType::STYPE_MIRROR &&
           data_.type <= ShipType::OTYPE_TRACT;
  }

  /// Whether ship has sensor visibility / crew sight range.
  [[nodiscard]] bool has_sight() const noexcept {
    return (data_.type == ShipType::OTYPE_PROBE) || data_.popn > 0;
  }

  /// Effective armor accounting for factory overrides and structural damage.
  [[nodiscard]] armor_t effective_armor() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<armor_t>(Shipdata[data_.type][ABIL_ARMOR])
               : static_cast<armor_t>(data_.armor * (100 - data_.damage) / 100);
  }

  /// Active weapon battery strength based on selected gun mode.
  [[nodiscard]] weapon_power_t active_guns() const noexcept {
    return (data_.guns == GTYPE_NONE)
               ? 0
               : (data_.guns == PRIMARY ? data_.primary : data_.secondary);
  }

  /// Structural body size excluding maximum hangar bay space.
  [[nodiscard]] int shipbody() const noexcept {
    return std::max(0, static_cast<int>(data_.size) -
                           static_cast<int>(data_.max_hanger));
  }

  /// Remaining available hangar space for docking smaller craft.
  [[nodiscard]] hangar_t hanger_space() const noexcept {
    return std::max(0L, static_cast<long>(data_.max_hanger) -
                            static_cast<long>(data_.hanger));
  }

  /// Available civilian crew capacity accounting for military troops on board.
  [[nodiscard]] population_t available_crew() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<population_t>(Shipdata[data_.type][ABIL_MAXCREW] -
                                           data_.troops)
               : (data_.max_crew - data_.troops);
  }

  /// Available military troop capacity accounting for civilian crew on board.
  [[nodiscard]] population_t available_mil() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<population_t>(Shipdata[data_.type][ABIL_MAXCREW] -
                                           data_.popn)
               : (data_.max_crew - data_.popn);
  }

  /// Maximum total crew capacity including factory template overrides.
  [[nodiscard]] population_t max_crew_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<population_t>(Shipdata[data_.type][ABIL_MAXCREW])
               : data_.max_crew;
  }

  /// Maximum cargo resource capacity including factory template overrides.
  [[nodiscard]] resource_t max_resource_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<resource_t>(Shipdata[data_.type][ABIL_CARGO])
               : data_.max_resource;
  }

  /// Maximum fuel tank capacity including factory template overrides.
  [[nodiscard]] unsigned short max_fuel_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<unsigned short>(Shipdata[data_.type][ABIL_FUELCAP])
               : data_.max_fuel;
  }

  /// Maximum ammo / ordnance capacity including factory template overrides.
  [[nodiscard]] unsigned short max_destruct_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<unsigned short>(Shipdata[data_.type][ABIL_DESTCAP])
               : data_.max_destruct;
  }

  /// Maximum impulse engine speed throttle including factory template
  /// overrides.
  [[nodiscard]] speed_t max_speed_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<speed_t>(Shipdata[data_.type][ABIL_SPEED])
               : data_.max_speed;
  }

  /// Maximum alien power crystal storage capacity.
  [[nodiscard]] int max_crystals_capacity() const noexcept {
    return MAX_CRYSTALS;
  }

  /// Repair work capacity per turn (operational status for factories, crew for
  /// others).
  [[nodiscard]] long repair_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY) ? data_.on
                                                   : available_crew();
  }

  /// Effective build / maintenance cost including factory activation scaling.
  [[nodiscard]] long effective_cost() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? 2L * data_.build_cost * data_.on +
                     Shipdata[data_.type][ABIL_COST]
               : data_.build_cost;
  }

  // =========================================================================
  // SERIALIZATION SUPPORT
  // =========================================================================

  // For repository serialization - returns copy of internal struct
  [[nodiscard]] virtual ship_struct get_struct() const {
    return to_struct();
  }

  // Direct access to internal struct (FOR SERIALIZATION USE ONLY)
  [[nodiscard]] virtual ship_struct to_struct() const {
    return data_;
  }
  [[nodiscard]] ship_struct& to_struct() noexcept {
    return data_;
  }
};

// =========================================================================
// AutonomousShip and Derived Specialty Subclasses
// =========================================================================

export class AutonomousShip : public Ship {
public:
  AutonomousShip() = default;
  explicit AutonomousShip(ship_struct in) : Ship(std::move(in)) {
    if (std::holds_alternative<MindData>(data_.special)) {
      mind_ = std::get<MindData>(data_.special);
    } else {
      mind_ =
          MindData{.progenitor = data_.owner, .generation = 1, .busy = true};
    }
  }

  [[nodiscard]] MindData& mind() noexcept {
    return mind_;
  }
  [[nodiscard]] const MindData& mind() const noexcept {
    return mind_;
  }
  [[nodiscard]] bool is_busy() const noexcept {
    return mind_.busy;
  }
  void set_busy(bool busy) noexcept {
    mind_.busy = busy;
  }

  [[nodiscard]] player_t progenitor() const noexcept {
    return mind_.progenitor;
  }
  [[nodiscard]] player_t target() const noexcept {
    return mind_.target;
  }
  void set_target(player_t target) noexcept {
    mind_.target = target;
  }
  [[nodiscard]] std::uint32_t generation() const noexcept {
    return mind_.generation;
  }

  [[nodiscard]] ship_struct to_struct() const override {
    ship_struct copy = data_;
    copy.special = mind_;
    return copy;
  }

protected:
  MindData mind_{};
};

export class VonNeumannShip : public AutonomousShip {
public:
  using AutonomousShip::AutonomousShip;
};

export class BerserkerShip : public AutonomousShip {
public:
  using AutonomousShip::AutonomousShip;
};

// Type traits for zero-cost static downcasting
export template <typename T>
struct ShipTypeTraits {
  static_assert(std::is_base_of_v<Ship, T>, "T must derive from Ship");
};

export template <>
struct ShipTypeTraits<AutonomousShip> {
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type == ShipType::OTYPE_VN || type == ShipType::OTYPE_BERS;
  }
};

export template <>
struct ShipTypeTraits<VonNeumannShip> {
  static constexpr ShipType expected_type = ShipType::OTYPE_VN;
};

export template <>
struct ShipTypeTraits<BerserkerShip> {
  static constexpr ShipType expected_type = ShipType::OTYPE_BERS;
};

template <typename Derived>
Derived* Ship::as() noexcept {
  static_assert(std::is_base_of_v<Ship, Derived>,
                "Derived must inherit from Ship");
  if constexpr (requires { ShipTypeTraits<Derived>::matches(type()); }) {
    if (ShipTypeTraits<Derived>::matches(type())) {
      return static_cast<Derived*>(this);
    }
  } else {
    if (type() == ShipTypeTraits<Derived>::expected_type) {
      return static_cast<Derived*>(this);
    }
  }
  return nullptr;
}

template <typename Derived>
const Derived* Ship::as() const noexcept {
  static_assert(std::is_base_of_v<Ship, Derived>,
                "Derived must inherit from Ship");
  if constexpr (requires { ShipTypeTraits<Derived>::matches(type()); }) {
    if (ShipTypeTraits<Derived>::matches(type())) {
      return static_cast<const Derived*>(this);
    }
  } else {
    if (type() == ShipTypeTraits<Derived>::expected_type) {
      return static_cast<const Derived*>(this);
    }
  }
  return nullptr;
}

export int getdefense(EntityManager&, const Ship&);
export void capture_stuff(const Ship&, GameObj&);
export double cost(const Ship&);
export double getmass(const Ship&);
export unsigned int ship_size(const Ship&);
export double complexity(const Ship&);
export double complexity(ShipType);  // Complexity for default ship of this type
export bool testship(const Ship&, GameObj&);
export std::tuple<bool, int> crash(const Ship& s, const double fuel) noexcept;
export void do_VN(EntityManager&, Ship&, TurnStats&);
export std::optional<player_t>
select_victim_to_steal_from(const Planet& planet,
                            std::span<const player_t> race_order);
export void planet_doVN(Ship&, Planet&, SectorMap&, EntityManager&, TurnStats&);
export void use_fuel(Ship&, double);
export void use_destruct(Ship&, int);
export void use_resource(Ship&, int);
export void rcv_fuel(Ship&, double);
export void rcv_resource(Ship&, int);
export void rcv_destruct(Ship&, int);
export void rcv_popn(Ship&, int, double);
export void rcv_troops(Ship&, int, double);
export std::string prin_ship_orbits(EntityManager&, const Ship&);
export std::string prin_ship_dest(const Ship&);
export void moveship(EntityManager&, Ship& ship, int x, int y, int z);
export void msg_OOF(EntityManager&, const Ship& ship);
export bool followable(EntityManager&, const Ship& ship, const Ship& target);

export shipnum_t Num_ships;

export Ship** ships;

export std::string dispshiploc_brief(EntityManager&, const Ship&);
export std::string dispshiploc(EntityManager&, const Ship&);

export const char Shipltrs[] = {
    'p', 's', 'X', 'D', 'B', 'I', 'C', 'd',  'f', 'e', 'H', 'S',
    'O', 'c', 't', '!', '+', 'M', '=', '\\', '-', 'a', 'g', 'h',
    'v', 'V', '@', 'l', 'w', ':', 'G', 'F',  'T', ';', 'Z', '[',
    '^', 'P', 'q', 'K', 'Y', 'W', 'J', '&',  'R', 'b', 'L'};

export template <>
struct std::formatter<Ship> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const Ship& s, auto& ctx) const {
    return std::format_to(ctx.out(), "{}{}{} [{}]", Shipltrs[s.type()],
                          s.number(), s.name(), s.owner());
  }
};

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
