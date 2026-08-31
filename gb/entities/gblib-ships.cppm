// SPDX-License-Identifier: Apache-2.0

/// \file gblib-ships.cppm
/// \brief Module interface partition for Ship domain entities, types, and
/// stats.

export module gblib:ships;

import std;

import :gameobj;
import :planet;
import :sector;
import :tweakables;
import :turnstats;

export enum guntype_t {
  GTYPE_NONE,
  GTYPE_LIGHT,
  GTYPE_MEDIUM,
  GTYPE_HEAVY
};

export enum class ActiveBattery : std::uint8_t {
  NONE = 0,
  PRIMARY = 1,
  SECONDARY = 2,
};

export inline constexpr ActiveBattery PRIMARY = ActiveBattery::PRIMARY;
export inline constexpr ActiveBattery SECONDARY = ActiveBattery::SECONDARY;

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
  Coordinates coords{0, 0};
  bool scatter{false};
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

  population_t max_crew{0};    ///< Maximum crew capacity
  resource_t max_resource{0};  ///< Maximum resource cargo capacity
  resource_t max_destruct{0};  ///< Maximum destructive charge capacity
  fuel_t max_fuel{0.0};        ///< Maximum fuel tank capacity
  speed_t max_speed{0};        ///< Maximum engine impulse speed
  ShipType build_type{
      ShipType::STYPE_POD};  ///< Ship template type when constructed
  money_t build_cost{0};     ///< Construction cost in resources

  double base_mass{0.0};   ///< Empty hull baseline mass
  double tech{0.0};        ///< Construction technology level
  double complexity{0.0};  ///< Hull structural complexity rating

  resource_t destruct{0};     ///< Current carried destructive charges
  resource_t resource{0};     ///< Current carried resource cargo
  population_t popn{0};       ///< Current carried colonists / crew
  population_t troops{0};     ///< Current carried military troops
  std::uint32_t crystals{0};  ///< Current carried warp crystal charge

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

  bool merchant{false};                     ///< Commercial trade vessel status
  ActiveBattery guns{ActiveBattery::NONE};  ///< Active gun battery mode
  weapon_power_t primary{0};                ///< Primary battery weapon payload
  guntype_t primtype{GTYPE_NONE};           ///< Primary gun caliber type
  weapon_power_t secondary{0};    ///< Secondary battery weapon payload
  guntype_t sectype{GTYPE_NONE};  ///< Secondary gun caliber type

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

/// \brief Strongly-typed immutable specifications and capabilities for a ship
/// class.
export struct ShipTemplate {
  ShipType type{ShipType::STYPE_POD};
  std::string_view name;
  char letter{'p'};

  // Baseline capacities & numerical metrics
  double base_tech{0.0};    ///< Baseline technology requirement (ABIL_TECH)
  resource_t max_cargo{0};  ///< Maximum resource cargo capacity (ABIL_CARGO)
  hangar_t max_hangar{0};   ///< Maximum hangar capacity (ABIL_HANGER)
  resource_t max_destruct{
      0};  ///< Maximum destruct crystal capacity (ABIL_DESTCAP)
  gun_count_t max_guns{0};  ///< Number of gun mounts (ABIL_GUNS)
  weapon_power_t primary_power{
      0};  ///< Primary battery weapon rating (ABIL_PRIMARY)
  weapon_power_t secondary_power{
      0};                ///< Secondary battery weapon rating (ABIL_SECONDARY)
  fuel_t max_fuel{0.0};  ///< Maximum fuel tank capacity (ABIL_FUELCAP)
  population_t max_crew{
      0};  ///< Maximum crew accommodation capacity (ABIL_MAXCREW)
  armor_t base_armor{0};    ///< Baseline hull armor rating (ABIL_ARMOR)
  money_t build_cost{0};    ///< Base construction cost in currency (ABIL_COST)
  speed_t base_speed{0};    ///< Base engine throttle speed rating (ABIL_SPEED)
  damage_t base_damage{0};  ///< Base structural damage threshold (ABIL_DAMAGE)
  double build_time{0.0};   ///< Construction build time factor (ABIL_BUILD)
  double construction_cost{
      0.0};                   ///< Construction cost multiplier (ABIL_CONSTRUCT)
  bool can_modify{false};     ///< Can be customized / modified (ABIL_MOD)
  gun_count_t max_lasers{0};  ///< Laser mount capacity (ABIL_LASER)

  // Boolean capabilities & operational permissions
  bool can_mount{
      false};  ///< Can mount warp crystals for hyperjump (ABIL_MOUNT)
  bool can_hyperjump{false};  ///< Equipped with hyperjump drive (ABIL_JUMP)
  bool can_land{false};       ///< Capable of planetary landing (ABIL_CANLAND)
  bool has_switch{
      false};           ///< Has toggleable power/mode switch (ABIL_HASSWITCH)
  bool has_cew{false};  ///< Equipped with Concentrated Energy Weapon (ABIL_CEW)
  bool can_cloak{false};    ///< Equipped with cloaking device (ABIL_CLOAK)
  bool is_god_only{false};  ///< Restricted to deity/admin creation (ABIL_GOD)
  bool is_programmed{
      false};  ///< Autonomous / automated AI control (ABIL_PROGRAMMED)
  bool is_starport{false};  ///< Operates as a starport (ABIL_PORT)
  bool can_repair{false};   ///< Capable of self/fleet repair (ABIL_REPAIR)
  bool requires_maintenance{
      false};  ///< Incurs regular economic maintenance cost (ABIL_MAINTAIN)
};

export inline constexpr std::array<ShipTemplate, NUMSTYPES> ship_templates = {{
    // 0: STYPE_POD (Spore pod, 'p')
    {.type = ShipType::STYPE_POD,
     .name = "Spore pod",
     .letter = 'p',
     .base_tech = 0,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 1,
     .base_armor = 0,
     .build_cost = 1,
     .base_speed = 2,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 1: STYPE_SHUTTLE (Shuttle, 's')
    {.type = ShipType::STYPE_SHUTTLE,
     .name = "Shuttle",
     .letter = 's',
     .base_tech = 10,
     .max_cargo = 25,
     .max_hangar = 2,
     .max_destruct = 2,
     .max_guns = 1,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 10,
     .base_armor = 0,
     .build_cost = 2,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 4,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 2: STYPE_CARRIER (Carrier, 'X')
    {.type = ShipType::STYPE_CARRIER,
     .name = "Carrier",
     .letter = 'X',
     .base_tech = 250,
     .max_cargo = 600,
     .max_hangar = 200,
     .max_destruct = 800,
     .max_guns = 30,
     .primary_power = 3,
     .secondary_power = 2,
     .max_fuel = 1000,
     .max_crew = 30,
     .base_armor = 5,
     .build_cost = 30,
     .base_speed = 4,
     .base_damage = 50,
     .build_time = 20,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = false,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 3: STYPE_DREADNT (Dreadnaught, 'D')
    {.type = ShipType::STYPE_DREADNT,
     .name = "Dreadnaught",
     .letter = 'D',
     .base_tech = 300,
     .max_cargo = 500,
     .max_hangar = 10,
     .max_destruct = 500,
     .max_guns = 60,
     .primary_power = 3,
     .secondary_power = 2,
     .max_fuel = 500,
     .max_crew = 60,
     .base_armor = 10,
     .build_cost = 40,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 4: STYPE_BATTLE (Battleship, 'B')
    {.type = ShipType::STYPE_BATTLE,
     .name = "Battleship",
     .letter = 'B',
     .base_tech = 200,
     .max_cargo = 235,
     .max_hangar = 10,
     .max_destruct = 400,
     .max_guns = 30,
     .primary_power = 3,
     .secondary_power = 2,
     .max_fuel = 200,
     .max_crew = 30,
     .base_armor = 7,
     .build_cost = 20,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 5: STYPE_INTCPT (Interceptor, 'I')
    {.type = ShipType::STYPE_INTCPT,
     .name = "Interceptor",
     .letter = 'I',
     .base_tech = 150,
     .max_cargo = 110,
     .max_hangar = 5,
     .max_destruct = 120,
     .max_guns = 20,
     .primary_power = 2,
     .secondary_power = 2,
     .max_fuel = 200,
     .max_crew = 20,
     .base_armor = 3,
     .build_cost = 15,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 6: STYPE_CRUISER (Cruiser, 'C')
    {.type = ShipType::STYPE_CRUISER,
     .name = "Cruiser",
     .letter = 'C',
     .base_tech = 150,
     .max_cargo = 165,
     .max_hangar = 5,
     .max_destruct = 300,
     .max_guns = 20,
     .primary_power = 3,
     .secondary_power = 2,
     .max_fuel = 120,
     .max_crew = 20,
     .base_armor = 5,
     .build_cost = 10,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 7: STYPE_DESTROYER (Destroyer, 'd')
    {.type = ShipType::STYPE_DESTROYER,
     .name = "Destroyer",
     .letter = 'd',
     .base_tech = 100,
     .max_cargo = 110,
     .max_hangar = 5,
     .max_destruct = 120,
     .max_guns = 15,
     .primary_power = 2,
     .secondary_power = 2,
     .max_fuel = 80,
     .max_crew = 15,
     .base_armor = 3,
     .build_cost = 5,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 8: STYPE_FIGHTER (Fighter Group, 'f')
    {.type = ShipType::STYPE_FIGHTER,
     .name = "Fighter Group",
     .letter = 'f',
     .base_tech = 100,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 40,
     .max_guns = 20,
     .primary_power = 2,
     .secondary_power = 1,
     .max_fuel = 10,
     .max_crew = 1,
     .base_armor = 2,
     .build_cost = 1,
     .base_speed = 9,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 9: STYPE_EXPLORER (Explorer, 'e')
    {.type = ShipType::STYPE_EXPLORER,
     .name = "Explorer",
     .letter = 'e',
     .base_tech = 40,
     .max_cargo = 10,
     .max_hangar = 0,
     .max_destruct = 15,
     .max_guns = 5,
     .primary_power = 2,
     .secondary_power = 0,
     .max_fuel = 35,
     .max_crew = 5,
     .base_armor = 1,
     .build_cost = 2,
     .base_speed = 6,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 10: STYPE_HABITAT (Habitat, 'H')
    {.type = ShipType::STYPE_HABITAT,
     .name = "Habitat",
     .letter = 'H',
     .base_tech = 100,
     .max_cargo = 5000,
     .max_hangar = 10,
     .max_destruct = 500,
     .max_guns = 20,
     .primary_power = 2,
     .secondary_power = 1,
     .max_fuel = 2000,
     .max_crew = 2000,
     .base_armor = 3,
     .build_cost = 50,
     .base_speed = 4,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 18,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = false,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = true,
     .can_repair = true,
     .requires_maintenance = true},

    // 11: STYPE_STATION (Station, 'S')
    {.type = ShipType::STYPE_STATION,
     .name = "Station",
     .letter = 'S',
     .base_tech = 100,
     .max_cargo = 5000,
     .max_hangar = 10,
     .max_destruct = 250,
     .max_guns = 20,
     .primary_power = 2,
     .secondary_power = 0,
     .max_fuel = 2000,
     .max_crew = 50,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 4,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 6,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = false,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = true,
     .can_repair = true,
     .requires_maintenance = true},

    // 12: STYPE_OAP (Ob Asst Pltfrm, 'O')
    {.type = ShipType::STYPE_OAP,
     .name = "Ob Asst Pltfrm",
     .letter = 'O',
     .base_tech = 200,
     .max_cargo = 1400,
     .max_hangar = 20,
     .max_destruct = 1000,
     .max_guns = 50,
     .primary_power = 3,
     .secondary_power = 2,
     .max_fuel = 2000,
     .max_crew = 200,
     .base_armor = 5,
     .build_cost = 40,
     .base_speed = 4,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 6,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = false,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 13: STYPE_CARGO (Cargo Ship, 'c')
    {.type = ShipType::STYPE_CARGO,
     .name = "Cargo Ship",
     .letter = 'c',
     .base_tech = 100,
     .max_cargo = 1000,
     .max_hangar = 5,
     .max_destruct = 1000,
     .max_guns = 10,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 1000,
     .max_crew = 100,
     .base_armor = 2,
     .build_cost = 10,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 4,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 14: STYPE_TANKER (Tanker, 't')
    {.type = ShipType::STYPE_TANKER,
     .name = "Tanker",
     .letter = 't',
     .base_tech = 100,
     .max_cargo = 200,
     .max_hangar = 5,
     .max_destruct = 200,
     .max_guns = 10,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 5000,
     .max_crew = 10,
     .base_armor = 2,
     .build_cost = 10,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 15: STYPE_GOD (GODSHIP, '!')
    {.type = ShipType::STYPE_GOD,
     .name = "GODSHIP",
     .letter = '!',
     .base_tech = 9999,
     .max_cargo = 20000,
     .max_hangar = 1000,
     .max_destruct = 20000,
     .max_guns = 1000,
     .primary_power = 3,
     .secondary_power = 3,
     .max_fuel = 20000,
     .max_crew = 1000,
     .base_armor = 100,
     .build_cost = 10,
     .base_speed = 9,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 6,
     .can_modify = true,
     .max_lasers = 1,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = true,
     .is_programmed = true,
     .is_starport = true,
     .can_repair = true,
     .requires_maintenance = false},

    // 16: STYPE_MINE (Space Mine, '+')
    {.type = ShipType::STYPE_MINE,
     .name = "Space Mine",
     .letter = '+',
     .base_tech = 50,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 25,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 0,
     .base_armor = 1,
     .build_cost = 30,
     .base_speed = 2,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 17: STYPE_MIRROR (Space Mirror, 'M')
    {.type = ShipType::STYPE_MIRROR,
     .name = "Space Mirror",
     .letter = 'M',
     .base_tech = 100,
     .max_cargo = 200,
     .max_hangar = 0,
     .max_destruct = 10,
     .max_guns = 1,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 5,
     .base_armor = 0,
     .build_cost = 100,
     .base_speed = 2,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = false,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 18: OTYPE_STELE (Space Telescope, '=')
    {.type = ShipType::OTYPE_STELE,
     .name = "Space Telescope",
     .letter = '=',
     .base_tech = 50,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 2,
     .base_armor = 0,
     .build_cost = 20,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = true},

    // 19: OTYPE_GTELE (Ground Telescope, '\')
    {.type = ShipType::OTYPE_GTELE,
     .name = "Ground Telescope",
     .letter = '\\',
     .base_tech = 5,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 2,
     .base_armor = 0,
     .build_cost = 2,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 20: OTYPE_TRACT (* T-R beam, '-')
    {.type = ShipType::OTYPE_TRACT,
     .name = "* T-R beam",
     .letter = '-',
     .base_tech = 200,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 1000,
     .max_crew = 5,
     .base_armor = 0,
     .build_cost = 20,
     .base_speed = 2,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = true,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = false,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 21: OTYPE_AP (Atmosph Processor, 'a')
    {.type = ShipType::OTYPE_AP,
     .name = "Atmosph Processor",
     .letter = 'a',
     .base_tech = 80,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 200,
     .max_crew = 10,
     .base_armor = 1,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 22: OTYPE_CANIST (Dust Canister, 'g')
    {.type = ShipType::OTYPE_CANIST,
     .name = "Dust Canister",
     .letter = 'g',
     .base_tech = 40,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 1,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 10,
     .base_speed = 1,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 23: OTYPE_GREEN (Greenhouse Gases, 'h')
    {.type = ShipType::OTYPE_GREEN,
     .name = "Greenhouse Gases",
     .letter = 'h',
     .base_tech = 40,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 1,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 10,
     .base_speed = 1,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 24: OTYPE_VN (V.Neumann Machine, 'v')
    {.type = ShipType::OTYPE_VN,
     .name = "V.Neumann Machine",
     .letter = 'v',
     .base_tech = 80,
     .max_cargo = 20,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 50,
     .max_crew = 0,
     .base_armor = 1,
     .build_cost = 100,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 25: OTYPE_BERS (Berserker, 'V')
    {.type = ShipType::OTYPE_BERS,
     .name = "Berserker",
     .letter = 'V',
     .base_tech = 999,
     .max_cargo = 50,
     .max_hangar = 0,
     .max_destruct = 500,
     .max_guns = 40,
     .primary_power = 3,
     .secondary_power = 2,
     .max_fuel = 1000,
     .max_crew = 0,
     .base_armor = 15,
     .build_cost = 100,
     .base_speed = 6,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = true,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 26: OTYPE_GOV (Govrnmnt. Center, '@')
    {.type = ShipType::OTYPE_GOV,
     .name = "Govrnmnt. Center",
     .letter = '@',
     .base_tech = 0,
     .max_cargo = 500,
     .max_hangar = 0,
     .max_destruct = 100,
     .max_guns = 10,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 1000,
     .max_crew = 10,
     .base_armor = 20,
     .build_cost = 500,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = true,
     .can_repair = true,
     .requires_maintenance = false},

    // 27: OTYPE_OMCL (Mind Control Lsr, 'l')
    {.type = ShipType::OTYPE_OMCL,
     .name = "Mind Control Lsr",
     .letter = 'l',
     .base_tech = 350,
     .max_cargo = 25,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 100,
     .max_crew = 2,
     .base_armor = 1,
     .build_cost = 50,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = false,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 28: OTYPE_TOXWC (Tox Waste Canistr, 'w')
    {.type = ShipType::OTYPE_TOXWC,
     .name = "Tox Waste Canistr",
     .letter = 'w',
     .base_tech = 0,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 5,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 29: OTYPE_PROBE (Space Probe, ':')
    {.type = ShipType::OTYPE_PROBE,
     .name = "Space Probe",
     .letter = ':',
     .base_tech = 150,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 10,
     .base_speed = 9,
     .base_damage = 0,
     .build_time = 19,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 30: OTYPE_GR (Gamma Ray Laser, 'G')
    {.type = ShipType::OTYPE_GR,
     .name = "Gamma Ray Laser",
     .letter = 'G',
     .base_tech = 100,
     .max_cargo = 50,
     .max_hangar = 0,
     .max_destruct = 120,
     .max_guns = 20,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 40,
     .base_armor = 3,
     .build_cost = 30,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = true,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 31: OTYPE_FACTORY (Factory, 'F')
    {.type = ShipType::OTYPE_FACTORY,
     .name = "Factory",
     .letter = 'F',
     .base_tech = 0,
     .max_cargo = 50,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 20,
     .base_armor = 0,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 17,
     .construction_cost = 8,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 32: OTYPE_TERRA (Terraform Device, 'T')
    {.type = ShipType::OTYPE_TERRA,
     .name = "Terraform Device",
     .letter = 'T',
     .base_tech = 50,
     .max_cargo = 40,
     .max_hangar = 5,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 200,
     .max_crew = 20,
     .base_armor = 1,
     .build_cost = 20,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 33: OTYPE_BERSCTLC (Bers Cntrl Center, ';')
    {.type = ShipType::OTYPE_BERSCTLC,
     .name = "Bers Cntrl Center",
     .letter = ';',
     .base_tech = 9999,
     .max_cargo = 200,
     .max_hangar = 0,
     .max_destruct = 50,
     .max_guns = 0,
     .primary_power = 3,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 0,
     .base_armor = 10,
     .build_cost = 3,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = true,
     .is_programmed = false,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 34: OTYPE_AUTOFAC (Bers Autofac, 'Z')
    {.type = ShipType::OTYPE_AUTOFAC,
     .name = "Bers Autofac",
     .letter = 'Z',
     .base_tech = 9999,
     .max_cargo = 1000,
     .max_hangar = 0,
     .max_destruct = 1000,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 1000,
     .max_crew = 0,
     .base_armor = 10,
     .build_cost = 8,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = true,
     .is_programmed = false,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 35: OTYPE_TRANSDEV (AVPM Transporter, '[')
    {.type = ShipType::OTYPE_TRANSDEV,
     .name = "AVPM Transporter",
     .letter = '[',
     .base_tech = 200,
     .max_cargo = 1000,
     .max_hangar = 0,
     .max_destruct = 1000,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 1000,
     .max_crew = 100,
     .base_armor = 0,
     .build_cost = 300,
     .base_speed = 0,
     .base_damage = 50,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 36: STYPE_MISSILE (Missile, '^')
    {.type = ShipType::STYPE_MISSILE,
     .name = "Missile",
     .letter = '^',
     .base_tech = 50,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 10,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 5,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 5,
     .base_speed = 6,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = false,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 37: OTYPE_PLANDEF (Planet Def Net, 'P')
    {.type = ShipType::OTYPE_PLANDEF,
     .name = "Planet Def Net",
     .letter = 'P',
     .base_tech = 200,
     .max_cargo = 50,
     .max_hangar = 0,
     .max_destruct = 500,
     .max_guns = 20,
     .primary_power = 3,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 50,
     .base_armor = 10,
     .build_cost = 100,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 38: OTYPE_QUARRY (Quarry, 'q')
    {.type = ShipType::OTYPE_QUARRY,
     .name = "Quarry",
     .letter = 'q',
     .base_tech = 0,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 200,
     .max_crew = 50,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 39: OTYPE_PLOW (Space Plow, 'K')
    {.type = ShipType::OTYPE_PLOW,
     .name = "Space Plow",
     .letter = 'K',
     .base_tech = 5,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 200,
     .max_crew = 10,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = false,
     .requires_maintenance = false},

    // 40: OTYPE_DOME (Dome, 'Y')
    {.type = ShipType::OTYPE_DOME,
     .name = "Dome",
     .letter = 'Y',
     .base_tech = 10,
     .max_cargo = 100,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 20,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 41: OTYPE_WPLANT (Weapons Plant, 'W')
    {.type = ShipType::OTYPE_WPLANT,
     .name = "Weapons Plant",
     .letter = 'W',
     .base_tech = 0,
     .max_cargo = 500,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 500,
     .max_crew = 20,
     .base_armor = 5,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = false,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = false},

    // 42: OTYPE_PORT (Space Port, 'J')
    {.type = ShipType::OTYPE_PORT,
     .name = "Space Port",
     .letter = 'J',
     .base_tech = 0,
     .max_cargo = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 100,
     .base_armor = 3,
     .build_cost = 50,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = true,
     .can_repair = true,
     .requires_maintenance = true},

    // 43: OTYPE_ABM (ABM Battery, '&')
    {.type = ShipType::OTYPE_ABM,
     .name = "ABM Battery",
     .letter = '&',
     .base_tech = 100,
     .max_cargo = 5,
     .max_hangar = 0,
     .max_destruct = 50,
     .max_guns = 5,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 0,
     .max_crew = 5,
     .base_armor = 5,
     .build_cost = 50,
     .base_speed = 0,
     .base_damage = 50,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = true,
     .has_switch = true,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 44: OTYPE_AFV (Mech, 'R')
    {.type = ShipType::OTYPE_AFV,
     .name = "Mech",
     .letter = 'R',
     .base_tech = 50,
     .max_cargo = 5,
     .max_hangar = 0,
     .max_destruct = 20,
     .max_guns = 2,
     .primary_power = 1,
     .secondary_power = 0,
     .max_fuel = 20,
     .max_crew = 1,
     .base_armor = 2,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = false,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 45: OTYPE_BUNKER (Bunker, 'b')
    {.type = ShipType::OTYPE_BUNKER,
     .name = "Bunker",
     .letter = 'b',
     .base_tech = 10,
     .max_cargo = 100,
     .max_hangar = 20,
     .max_destruct = 100,
     .max_guns = 0,
     .primary_power = 0,
     .secondary_power = 0,
     .max_fuel = 100,
     .max_crew = 100,
     .base_armor = 15,
     .build_cost = 100,
     .base_speed = 0,
     .base_damage = 50,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = false,
     .can_hyperjump = false,
     .can_land = false,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},

    // 46: STYPE_LANDER (Lander, 'L')
    {.type = ShipType::STYPE_LANDER,
     .name = "Lander",
     .letter = 'L',
     .base_tech = 150,
     .max_cargo = 100,
     .max_hangar = 10,
     .max_destruct = 200,
     .max_guns = 10,
     .primary_power = 3,
     .secondary_power = 0,
     .max_fuel = 100,
     .max_crew = 500,
     .base_armor = 7,
     .build_cost = 50,
     .base_speed = 2,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .max_lasers = 0,
     .can_mount = true,
     .can_hyperjump = true,
     .can_land = true,
     .has_switch = false,
     .has_cew = false,
     .can_cloak = false,
     .is_god_only = false,
     .is_programmed = true,
     .is_starport = false,
     .can_repair = true,
     .requires_maintenance = true},
}};

/// \brief Returns immutable template specifications for a given ship class.
export [[nodiscard]] constexpr const ShipTemplate&
ship_template(ShipType type) noexcept {
  const auto idx = static_cast<std::size_t>(type);
  if (idx < ship_templates.size()) {
    return ship_templates[idx];
  }
  return ship_templates[0];
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

  [[nodiscard]] resource_t max_destruct() const {
    return data_.max_destruct;
  }
  resource_t& max_destruct() {
    return data_.max_destruct;
  }

  [[nodiscard]] fuel_t max_fuel() const {
    return data_.max_fuel;
  }
  fuel_t& max_fuel() {
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

  [[nodiscard]] money_t build_cost() const {
    return data_.build_cost;
  }
  money_t& build_cost() {
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
  [[nodiscard]] resource_t destruct() const {
    return data_.destruct;
  }
  resource_t& destruct() {
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

  [[nodiscard]] std::uint32_t crystals() const {
    return data_.crystals;
  }
  std::uint32_t& crystals() {
    return data_.crystals;
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

  /// \brief Returns the immutable class template for this ship's type.
  [[nodiscard]] constexpr const ShipTemplate& get_template() const noexcept {
    return ship_template(data_.type);
  }

  /// \brief Indicates whether this ship class has self/fleet repair capability.
  [[nodiscard]] constexpr bool can_repair() const noexcept {
    return get_template().can_repair;
  }

  /// \brief Indicates whether this ship class incurs ongoing economic
  /// maintenance costs.
  [[nodiscard]] constexpr bool requires_maintenance() const noexcept {
    return get_template().requires_maintenance;
  }

  /// \brief Indicates whether this ship class is capable of planetary landing.
  [[nodiscard]] constexpr bool can_land() const noexcept {
    return get_template().can_land;
  }

  /// \brief Indicates whether this ship class is equipped with a hyperjump
  /// drive.
  [[nodiscard]] constexpr bool can_hyperjump() const noexcept {
    return get_template().can_hyperjump;
  }

  /// \brief Indicates whether this ship class can be modified or customized.
  [[nodiscard]] constexpr bool can_modify() const noexcept {
    return get_template().can_modify;
  }

  /// \brief Indicates whether this ship class can mount warp crystals.
  [[nodiscard]] constexpr bool can_mount() const noexcept {
    return get_template().can_mount;
  }

  /// \brief Returns whether this ship is capable of exploring stars and planets
  /// (carrying population/crew or being an automated sensor probe).
  [[nodiscard]] constexpr bool is_exploration_capable() const noexcept {
    return data_.popn > 0 || data_.type == ShipType::OTYPE_PROBE;
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

  [[nodiscard]] ActiveBattery guns() const {
    return data_.guns;
  }
  ActiveBattery& guns() {
    return data_.guns;
  }
  [[nodiscard]] ActiveBattery active_battery() const {
    return data_.guns;
  }
  ActiveBattery& active_battery() {
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
    return (data_.guns == ActiveBattery::NONE)
               ? 0
               : (data_.guns == ActiveBattery::PRIMARY ? data_.primary
                                                       : data_.secondary);
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
               ? ship_template(data_.type).max_cargo
               : data_.max_resource;
  }

  /// Maximum fuel tank capacity including factory template overrides.
  [[nodiscard]] fuel_t max_fuel_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? ship_template(data_.type).max_fuel
               : data_.max_fuel;
  }

  /// Maximum ammo / ordnance capacity including factory template overrides.
  [[nodiscard]] resource_t max_destruct_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? ship_template(data_.type).max_destruct
               : data_.max_destruct;
  }

  /// Maximum impulse engine speed throttle including factory template
  /// overrides.
  [[nodiscard]] speed_t max_speed_capacity() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? ship_template(data_.type).base_speed
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

  /// Ship classification type letter code.
  [[nodiscard]] char type_letter() const noexcept {
    return get_template().letter;
  }

  /// Maximum gun mount capacity from ship template.
  [[nodiscard]] gun_count_t max_guns_capacity() const noexcept {
    return get_template().max_guns;
  }

  // =========================================================================
  // DOMAIN OPERATIONS & STATE TRANSITIONS
  // =========================================================================

  /// \brief Increases hull damage by the specified amount, clamped to 100%.
  void apply_damage(damage_t amt) noexcept {
    data_.damage = std::min<damage_t>(100, data_.damage + amt);
  }

  /// \brief Repairs hull damage by the specified amount, clamped to 0%.
  void repair_damage(damage_t amt) noexcept {
    data_.damage = (amt >= data_.damage) ? 0 : data_.damage - amt;
  }

  /// \brief Reduces accumulated radiation dose, clamped to 0.
  void repair_radiation(radiation_t amt) noexcept {
    data_.rad = (amt >= data_.rad) ? 0 : data_.rad - amt;
  }

  /// \brief Consumes fuel and decrements ship mass accordingly.
  void consume_fuel(fuel_t amt) noexcept {
    data_.fuel -= amt;
    data_.mass -= amt * MASS_FUEL;
  }

  /// \brief Adds fuel and increments ship mass accordingly.
  void add_fuel(fuel_t amt) noexcept {
    data_.fuel += amt;
    data_.mass += amt * MASS_FUEL;
  }

  /// \brief Consumes resources and decrements ship mass accordingly.
  void consume_resource(resource_t amt) noexcept {
    data_.resource -= amt;
    data_.mass -= static_cast<double>(amt) * MASS_RESOURCE;
  }

  /// \brief Adds resources and increments ship mass accordingly.
  void add_resource(resource_t amt) noexcept {
    data_.resource += amt;
    data_.mass += static_cast<double>(amt) * MASS_RESOURCE;
  }

  /// \brief Consumes destruct ordnance and decrements ship mass accordingly.
  void consume_destruct(resource_t amt) noexcept {
    data_.destruct -= static_cast<unsigned short>(amt);
    data_.mass -= static_cast<double>(amt) * MASS_DESTRUCT;
  }

  /// \brief Adds destruct ordnance and increments ship mass accordingly.
  void add_destruct(resource_t amt) noexcept {
    data_.destruct += static_cast<unsigned short>(amt);
    data_.mass += static_cast<double>(amt) * MASS_DESTRUCT;
  }

  /// \brief Adds population and increments ship mass based on race mass.
  void add_popn(population_t amt, double race_mass) noexcept {
    data_.popn += amt;
    data_.mass += static_cast<double>(amt) * race_mass;
  }

  /// \brief Adds troops and increments ship mass based on race mass.
  void add_troops(population_t amt, double race_mass) noexcept {
    data_.troops += amt;
    data_.mass += static_cast<double>(amt) * race_mass;
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
    if (!std::holds_alternative<MindData>(data_.special)) {
      data_.special =
          MindData{.progenitor = data_.owner, .generation = 1, .busy = true};
    }
  }

  [[nodiscard]] MindData& mind() noexcept {
    if (!std::holds_alternative<MindData>(data_.special)) {
      data_.special =
          MindData{.progenitor = data_.owner, .generation = 1, .busy = true};
    }
    return std::get<MindData>(data_.special);
  }
  [[nodiscard]] const MindData& mind() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special);
    }
    static const MindData default_mind{};
    return default_mind;
  }
  [[nodiscard]] bool is_busy() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special).busy;
    }
    return true;
  }
  void set_busy(bool busy) noexcept {
    mind().busy = busy;
  }

  [[nodiscard]] player_t progenitor() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special).progenitor;
    }
    return data_.owner;
  }
  [[nodiscard]] player_t target() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special).target;
    }
    return player_t{0};
  }
  void set_target(player_t target) noexcept {
    mind().target = target;
  }
  [[nodiscard]] player_t who_killed() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special).who_killed;
    }
    return player_t{0};
  }
  void set_who_killed(player_t killer) noexcept {
    mind().who_killed = killer;
  }
  [[nodiscard]] std::uint32_t generation() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special).generation;
    }
    return 1;
  }
  [[nodiscard]] bool is_tampered() const noexcept {
    if (std::holds_alternative<MindData>(data_.special)) {
      return std::get<MindData>(data_.special).tampered;
    }
    return false;
  }
  void set_tampered(bool tampered) noexcept {
    mind().tampered = tampered;
  }
};

export class VonNeumannShip : public AutonomousShip {
public:
  using AutonomousShip::AutonomousShip;
};

export class BerserkerShip : public AutonomousShip {
public:
  using AutonomousShip::AutonomousShip;
};

export class SpaceMirrorShip : public Ship {
public:
  SpaceMirrorShip() = default;
  explicit SpaceMirrorShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<AimedAtData>(data_.special)) {
      data_.special = AimedAtData{};
    }
  }

  [[nodiscard]] AimedAtData& aim() noexcept {
    if (!std::holds_alternative<AimedAtData>(data_.special)) {
      data_.special = AimedAtData{};
    }
    return std::get<AimedAtData>(data_.special);
  }
  [[nodiscard]] const AimedAtData& aim() const noexcept {
    if (std::holds_alternative<AimedAtData>(data_.special)) {
      return std::get<AimedAtData>(data_.special);
    }
    static const AimedAtData default_aim{};
    return default_aim;
  }
  [[nodiscard]] char intensity() const noexcept {
    return aim().intensity;
  }
  void set_intensity(char intensity) noexcept {
    aim().intensity = intensity;
  }
  [[nodiscard]] starnum_t aimed_star() const noexcept {
    return aim().snum;
  }
  [[nodiscard]] planetnum_t aimed_planet() const noexcept {
    return aim().pnum;
  }
  [[nodiscard]] shipnum_t aimed_ship() const noexcept {
    return aim().shipno;
  }
  [[nodiscard]] ScopeLevel aimed_level() const noexcept {
    return aim().level;
  }

  /// Resolves the absolute coordinates (x, y) of the aimed target.
  [[nodiscard]] std::optional<std::pair<double, double>>
  target_coordinates(EntityManager& em) const;

  /// Calculates the 0..7 compass aim direction heading toward the target.
  [[nodiscard]] int aim_direction(EntityManager& em) const;
};

export class SporePodShip : public Ship {
public:
  SporePodShip() = default;
  explicit SporePodShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<PodData>(data_.special)) {
      data_.special = PodData{};
    }
  }

  [[nodiscard]] PodData& pod() noexcept {
    if (!std::holds_alternative<PodData>(data_.special)) {
      data_.special = PodData{};
    }
    return std::get<PodData>(data_.special);
  }
  [[nodiscard]] const PodData& pod() const noexcept {
    if (std::holds_alternative<PodData>(data_.special)) {
      return std::get<PodData>(data_.special);
    }
    static const PodData default_pod{};
    return default_pod;
  }
  [[nodiscard]] unsigned char decay() const noexcept {
    return pod().decay;
  }
  void set_decay(unsigned char decay) noexcept {
    pod().decay = decay;
  }
  [[nodiscard]] unsigned char temperature() const noexcept {
    return pod().temperature;
  }
  void set_temperature(unsigned char temp) noexcept {
    pod().temperature = temp;
  }
};

export class CanisterShip : public Ship {
public:
  CanisterShip() = default;
  explicit CanisterShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<TimerData>(data_.special)) {
      data_.special = TimerData{};
    }
  }

  [[nodiscard]] TimerData& timer() noexcept {
    if (!std::holds_alternative<TimerData>(data_.special)) {
      data_.special = TimerData{};
    }
    return std::get<TimerData>(data_.special);
  }
  [[nodiscard]] const TimerData& timer() const noexcept {
    if (std::holds_alternative<TimerData>(data_.special)) {
      return std::get<TimerData>(data_.special);
    }
    static const TimerData default_timer{};
    return default_timer;
  }
  [[nodiscard]] unsigned char count() const noexcept {
    return timer().count;
  }
  void set_count(unsigned char count) noexcept {
    timer().count = count;
  }
  void reset_timer() noexcept {
    timer().count = 0;
  }
};

export class MissileShip : public Ship {
public:
  MissileShip() = default;
  explicit MissileShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<ImpactData>(data_.special)) {
      data_.special = ImpactData{};
    }
  }

  [[nodiscard]] ImpactData& impact() noexcept {
    if (!std::holds_alternative<ImpactData>(data_.special)) {
      data_.special = ImpactData{};
    }
    return std::get<ImpactData>(data_.special);
  }
  [[nodiscard]] const ImpactData& impact() const noexcept {
    if (std::holds_alternative<ImpactData>(data_.special)) {
      return std::get<ImpactData>(data_.special);
    }
    static const ImpactData default_impact{};
    return default_impact;
  }
  [[nodiscard]] Coordinates impact_coords() const noexcept {
    return impact().coords;
  }
  [[nodiscard]] bool is_scatter() const noexcept {
    return impact().scatter;
  }
  void set_impact_coords(Coordinates coords) noexcept {
    impact().coords = coords;
    impact().scatter = false;
  }
  void set_scatter() noexcept {
    impact().coords = Coordinates{0, 0};
    impact().scatter = true;
  }
};

export class MineShip : public Ship {
public:
  MineShip() = default;
  explicit MineShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<TriggerData>(data_.special)) {
      data_.special = TriggerData{};
    }
  }

  [[nodiscard]] TriggerData& trigger() noexcept {
    if (!std::holds_alternative<TriggerData>(data_.special)) {
      data_.special = TriggerData{};
    }
    return std::get<TriggerData>(data_.special);
  }
  [[nodiscard]] const TriggerData& trigger() const noexcept {
    if (std::holds_alternative<TriggerData>(data_.special)) {
      return std::get<TriggerData>(data_.special);
    }
    static const TriggerData default_trigger{};
    return default_trigger;
  }
  [[nodiscard]] unsigned short trigger_radius() const noexcept {
    return trigger().radius;
  }
  void set_trigger_radius(unsigned short radius) noexcept {
    trigger().radius = radius;
  }
  [[nodiscard]] bool is_radiative() const noexcept {
    return data_.mode;
  }
  void set_radiative(bool rad) noexcept {
    data_.mode = rad;
  }
};

export class TerraformerShip : public Ship {
public:
  TerraformerShip() = default;
  explicit TerraformerShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<TerraformData>(data_.special)) {
      data_.special = TerraformData{};
    }
  }

  [[nodiscard]] TerraformData& terraform() noexcept {
    if (!std::holds_alternative<TerraformData>(data_.special)) {
      data_.special = TerraformData{};
    }
    return std::get<TerraformData>(data_.special);
  }
  [[nodiscard]] const TerraformData& terraform() const noexcept {
    if (std::holds_alternative<TerraformData>(data_.special)) {
      return std::get<TerraformData>(data_.special);
    }
    static const TerraformData default_terraform{};
    return default_terraform;
  }
  [[nodiscard]] unsigned char index() const noexcept {
    return terraform().index;
  }
  void set_index(unsigned char idx) noexcept {
    terraform().index = idx;
  }
};

export class GroundPlowShip : public TerraformerShip {
public:
  using TerraformerShip::TerraformerShip;
};

export class TransporterShip : public Ship {
public:
  TransporterShip() = default;
  explicit TransporterShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<TransportData>(data_.special)) {
      data_.special = TransportData{};
    }
  }

  [[nodiscard]] TransportData& transport() noexcept {
    if (!std::holds_alternative<TransportData>(data_.special)) {
      data_.special = TransportData{};
    }
    return std::get<TransportData>(data_.special);
  }
  [[nodiscard]] const TransportData& transport() const noexcept {
    if (std::holds_alternative<TransportData>(data_.special)) {
      return std::get<TransportData>(data_.special);
    }
    static const TransportData default_transport{};
    return default_transport;
  }
  [[nodiscard]] shipnum_t target_ship() const noexcept {
    return shipnum_t{transport().target};
  }
  void set_target_ship(shipnum_t target) noexcept {
    transport().target = static_cast<unsigned short>(target.value);
  }
};

export class ToxicWasteShip : public Ship {
public:
  ToxicWasteShip() = default;
  explicit ToxicWasteShip(ship_struct in) : Ship(std::move(in)) {
    if (!std::holds_alternative<WasteData>(data_.special)) {
      data_.special = WasteData{};
    }
  }

  [[nodiscard]] WasteData& waste() noexcept {
    if (!std::holds_alternative<WasteData>(data_.special)) {
      data_.special = WasteData{};
    }
    return std::get<WasteData>(data_.special);
  }
  [[nodiscard]] const WasteData& waste() const noexcept {
    if (std::holds_alternative<WasteData>(data_.special)) {
      return std::get<WasteData>(data_.special);
    }
    static const WasteData default_waste{};
    return default_waste;
  }
  [[nodiscard]] unsigned char toxic_level() const noexcept {
    return waste().toxic;
  }
  void set_toxic_level(unsigned char toxic) noexcept {
    waste().toxic = toxic;
  }
};

static_assert(sizeof(AutonomousShip) == sizeof(Ship));
static_assert(sizeof(VonNeumannShip) == sizeof(Ship));
static_assert(sizeof(BerserkerShip) == sizeof(Ship));
static_assert(sizeof(SpaceMirrorShip) == sizeof(Ship));
static_assert(sizeof(SporePodShip) == sizeof(Ship));
static_assert(sizeof(CanisterShip) == sizeof(Ship));
static_assert(sizeof(MissileShip) == sizeof(Ship));
static_assert(sizeof(MineShip) == sizeof(Ship));
static_assert(sizeof(TerraformerShip) == sizeof(Ship));
static_assert(sizeof(GroundPlowShip) == sizeof(Ship));
static_assert(sizeof(TransporterShip) == sizeof(Ship));
static_assert(sizeof(ToxicWasteShip) == sizeof(Ship));

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

export template <>
struct ShipTypeTraits<SpaceMirrorShip> {
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type >= ShipType::STYPE_MIRROR && type <= ShipType::OTYPE_TRACT;
  }
};

export template <>
struct ShipTypeTraits<SporePodShip> {
  static constexpr ShipType expected_type = ShipType::STYPE_POD;
};

export template <>
struct ShipTypeTraits<CanisterShip> {
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type == ShipType::OTYPE_CANIST || type == ShipType::OTYPE_GREEN;
  }
};

export template <>
struct ShipTypeTraits<MissileShip> {
  static constexpr ShipType expected_type = ShipType::STYPE_MISSILE;
};

export template <>
struct ShipTypeTraits<MineShip> {
  static constexpr ShipType expected_type = ShipType::STYPE_MINE;
};

export template <>
struct ShipTypeTraits<TerraformerShip> {
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type == ShipType::OTYPE_TERRA || type == ShipType::OTYPE_PLOW;
  }
};

export template <>
struct ShipTypeTraits<GroundPlowShip> {
  static constexpr ShipType expected_type = ShipType::OTYPE_PLOW;
};

export template <>
struct ShipTypeTraits<TransporterShip> {
  static constexpr ShipType expected_type = ShipType::OTYPE_TRANSDEV;
};

export template <>
struct ShipTypeTraits<ToxicWasteShip> {
  static constexpr ShipType expected_type = ShipType::OTYPE_TOXWC;
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
export void use_fuel(Ship&, fuel_t);
export void use_destruct(Ship&, resource_t);
export void use_resource(Ship&, resource_t);
export void rcv_fuel(Ship&, fuel_t);
export void rcv_resource(Ship&, resource_t);
export void rcv_destruct(Ship&, resource_t);
export void rcv_popn(Ship&, population_t, double);
export void rcv_troops(Ship&, population_t, double);
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

export template <std::derived_from<Ship> T>
struct std::formatter<T> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const T& s, auto& ctx) const {
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
