// SPDX-License-Identifier: Apache-2.0

/// \file gblib-ships.cppm
/// \brief Module interface partition for Ship domain entities, types, and
/// stats.

export module gblib:ships;

import std;

import :gameobj;
import :planet;
import :sector;
import :types;
import :tweakables;
import :turnstats;

export enum class guntype_t : std::uint8_t {
  NONE = 0,
  LIGHT = 1,
  MEDIUM = 2,
  HEAVY = 3,
};

/// \brief Returns the integer caliber multiplier for combat and mass
/// calculations.
export [[nodiscard]] constexpr unsigned int
gun_caliber(guntype_t caliber) noexcept {
  switch (caliber) {
    case guntype_t::LIGHT:
      return 1;
    case guntype_t::MEDIUM:
      return 2;
    case guntype_t::HEAVY:
      return 3;
    case guntype_t::NONE:
      return 0;
  }
}

/// Get display character for gun caliber type
/// \param caliber Gun caliber type (guntype_t::NONE=0, guntype_t::LIGHT=1,
/// guntype_t::MEDIUM=2, guntype_t::HEAVY=3)
/// \return Character representing caliber ('L', 'M', 'H', or ' ' for none)
export constexpr char caliber_char(guntype_t caliber) {
  switch (caliber) {
    case guntype_t::LIGHT:
      return 'L';
    case guntype_t::MEDIUM:
      return 'M';
    case guntype_t::HEAVY:
      return 'H';
    case guntype_t::NONE:
    default:
      return ' ';
  }
}

export enum class ActiveBattery : std::uint8_t {
  NONE = 0,
  PRIMARY = 1,
  SECONDARY = 2,
};

/// \brief Value object representing a ship's gun battery mount, encapsulating
/// weapon count and caliber while enforcing domain invariants.
///
/// Invariants enforced:
/// - A battery with 0 guns has caliber guntype_t::NONE.
/// - A battery with guntype_t::NONE caliber has 0 guns.
/// - count > 0 if and only if caliber != guntype_t::NONE.
export struct GunBattery {
  gun_count_t count{0};
  guntype_t caliber{guntype_t::NONE};

  /// \brief Factory method that normalizes empty states to count 0 and caliber
  /// NONE.
  [[nodiscard]] static constexpr GunBattery create(gun_count_t count,
                                                   guntype_t caliber) noexcept {
    if (count == 0 || caliber == guntype_t::NONE) {
      return {.count = 0, .caliber = guntype_t::NONE};
    }
    return {.count = count, .caliber = caliber};
  }

  /// \brief Returns true if the battery has guns mounted and a valid caliber.
  [[nodiscard]] constexpr bool has_guns() const noexcept {
    return count > 0 && caliber != guntype_t::NONE;
  }

  /// \brief Returns true if the battery is empty (0 guns or caliber NONE).
  [[nodiscard]] constexpr bool is_empty() const noexcept {
    return !has_guns();
  }

  /// \brief Returns the integer caliber multiplier for mass and combat
  /// calculations (0 for empty/NONE, 1 for LIGHT, 2 for MEDIUM, 3 for HEAVY).
  [[nodiscard]] constexpr unsigned int caliber_multiplier() const noexcept {
    return has_guns() ? gun_caliber(caliber) : 0u;
  }

  /// \brief Returns the mass contribution of this battery (count * caliber
  /// multiplier).
  [[nodiscard]] constexpr double mass_contribution() const noexcept {
    return static_cast<double>(count) *
           static_cast<double>(caliber_multiplier());
  }

  /// \brief Atomically sets count and caliber, maintaining domain invariants.
  constexpr void set(gun_count_t new_count, guntype_t new_caliber) noexcept {
    if (new_count == 0 || new_caliber == guntype_t::NONE) {
      count = 0;
      caliber = guntype_t::NONE;
    } else {
      count = new_count;
      caliber = new_caliber;
    }
  }

  /// \brief Applies combat collateral damage to gun mounts.
  /// \param hits Number of gun hits to apply.
  /// \return The actual number of guns destroyed (clamped to available count).
  /// If all guns are destroyed, the caliber is automatically cleared to NONE.
  constexpr gun_count_t damage(gun_count_t hits) noexcept {
    const gun_count_t lost = std::min(hits, count);
    count -= lost;
    if (count == 0) {
      caliber = guntype_t::NONE;
    }
    return lost;
  }

  /// \brief Formats the battery as "<count><caliber_char>", e.g. "10L", or "0 "
  /// if empty.
  [[nodiscard]] std::string to_string() const {
    return std::format("{}{}", count, caliber_char(caliber));
  }

  constexpr auto operator<=>(const GunBattery&) const noexcept = default;
};

/// \brief Structured outcome of applying damage to a ship.
export struct DamageResult {
  damage_t damage_applied{0};  ///< Actual damage added (clamped to 100 max)
  damage_t new_damage{0};      ///< New total damage percentage [0..100]
  bool destroyed{false};       ///< True if new_damage reached 100%
};

export inline constexpr ActiveBattery PRIMARY = ActiveBattery::PRIMARY;
export inline constexpr ActiveBattery SECONDARY = ActiveBattery::SECONDARY;

/// \brief Operational and structural ship type classifications.
///
/// Prefix conventions:
/// - `STYPE_*`: Mobile spacefaring vessels, orbital stations, and tactical
/// craft.
/// - `OTYPE_*`: Planetary installations, ground structures, specialized orbital
/// devices, and terraforming machinery.
export enum ShipType : int {
  /// \brief Biological spore pod ('p') launched to seed meta-colonies across
  /// star systems.
  STYPE_POD,
  /// \brief Light sublight personnel transport and planetary shuttle ('s').
  STYPE_SHUTTLE,
  /// \brief Heavy fleet capital carrier ('X') equipped with massive hangar bays
  /// for carried craft.
  STYPE_CARRIER,
  /// \brief Super-heavy dreadnaught battleship ('D') featuring heavy armor and
  /// dual gun batteries.
  STYPE_DREADNT,
  /// \brief Front-line capital battleship ('B') designed for sustained orbital
  /// and deep-space combat.
  STYPE_BATTLE,
  /// \brief Fast high-acceleration interceptor ('I') optimized for patrol and
  /// dogfighting.
  STYPE_INTCPT,
  /// \brief Medium multi-role combat cruiser ('C') balancing firepower, armor,
  /// and range.
  STYPE_CRUISER,
  /// \brief Fleet escort destroyer ('d') specialized for anti-fighter and
  /// screening operations.
  STYPE_DESTROYER,
  /// \brief Parasite fighter group ('f') carried in hangars for short-range
  /// combat sorties.
  STYPE_FIGHTER,
  /// \brief Long-range sensor explorer ('e') built for stellar reconnaissance
  /// and mapping.
  STYPE_EXPLORER,
  /// \brief Massive self-contained orbital habitat ('H') housing civilian
  /// population and nested factories.
  STYPE_HABITAT,
  /// \brief Heavy orbital space station ('S') serving as a military redoubt or
  /// logistical hub.
  STYPE_STATION,
  /// \brief Orbital Assault Platform ('O') providing orbital bombardment and
  /// surface intimidation.
  STYPE_OAP,
  /// \brief Heavy cargo transport ('c') built for bulk commodity and resource
  /// hauling.
  STYPE_CARGO,
  /// \brief Specialized liquid fuel transport ('t') supplying operational
  /// fleets.
  STYPE_TANKER,
  /// \brief Invulnerable deity administration vessel ('!') with infinite fuel
  /// and ordnance recharge.
  STYPE_GOD,
  /// \brief Autonomous proximity space mine ('+') detonating when hostile
  /// vessels enter detection radius.
  STYPE_MINE,
  /// \brief Orbital space mirror ('M') focusing stellar radiation to warm
  /// planets or scorch targets.
  STYPE_MIRROR,
  /// \brief Orbital space telescope ('=') providing long-range stellar
  /// surveillance.
  OTYPE_STELE,
  /// \brief Surface-based ground telescope ('\') providing deep-space tracking
  /// from planetary soil.
  OTYPE_GTELE,
  /// \brief Orbital tractor-repulsor beam projector ('-') manipulating local
  /// ship vectors.
  OTYPE_TRACT,
  /// \brief Atmospheric processor ('a') actively terraforming atmospheric
  /// composition over time.
  OTYPE_AP,
  /// \brief Atmospheric dust canister ('g') cooling planetary climates through
  /// aerosol dispersal.
  OTYPE_CANIST,
  /// \brief Greenhouse gas canister ('h') warming planetary climates through
  /// thermal trapping.
  OTYPE_GREEN,
  /// \brief Self-replicating Von Neumann machine ('v') harvesting resources to
  /// build planetary copies.
  OTYPE_VN,
  /// \brief Rogue autonomous Berserker war machine ('V') hunting and destroying
  /// alien life.
  OTYPE_BERS,
  /// \brief Planetary Government Center ('@') anchoring planetary
  /// administration and sector taxation.
  OTYPE_GOV,
  /// \brief Orbital Mind Control Laser ('l') projecting pacification beams onto
  /// rebellious populations.
  OTYPE_OMCL,
  /// \brief Hazardous toxic waste canister ('w') dumping nuclear and chemical
  /// slag onto planet surfaces.
  OTYPE_TOXWC,
  /// \brief Unmanned automated space probe (':') for expendable planetary and
  /// system reconnaissance.
  OTYPE_PROBE,
  /// \brief Orbital gamma-ray laser weapon ('G') delivering high-energy
  /// directed-energy surface strikes.
  OTYPE_GR,
  /// \brief Surface industrial manufacturing factory ('F') producing weapons
  /// and consumer goods.
  OTYPE_FACTORY,
  /// \brief Mobile planetary terraforming device ('T') engineering surface soil
  /// and vegetation.
  OTYPE_TERRA,
  /// \brief Berserker control center (';') coordinating autonomous machine
  /// fleets.
  OTYPE_BERSCTLC,
  /// \brief Automated Berserker assembly complex ('Z') continuously fabricating
  /// robotic war machines.
  OTYPE_AUTOFAC,
  /// \brief AVPM orbital matter transporter ('[') beaming cargo and resources
  /// directly to surfaces.
  OTYPE_TRANSDEV,
  /// \brief Guided orbital bombardment or anti-ship missile ('^') detonating on
  /// impact.
  STYPE_MISSILE,
  /// \brief Planetary surface defense battery network ('P') firing at hostile
  /// ships in orbit.
  OTYPE_PLANDEF,
  /// \brief Heavy surface mineral extraction quarry ('q') harvesting planetary
  /// metals and resources.
  OTYPE_QUARRY,
  /// \brief Surface-crawling mechanized space plow ('K') excavating and
  /// restructuring terrain sectors.
  OTYPE_PLOW,
  /// \brief Pressurized environmental biodome ('Y') shielding delicate
  /// colonists from hostile climates.
  OTYPE_DOME,
  /// \brief Heavy surface weapons plant ('W') fabricating ordnance and
  /// destructive munitions.
  OTYPE_WPLANT,
  /// \brief Commercial orbital spaceport ('J') facilitating interstellar trade
  /// and passenger embarkation.
  OTYPE_PORT,
  /// \brief Anti-Ballistic Missile battery ('&') intercepting incoming missiles
  /// and orbital strikes.
  OTYPE_ABM,
  /// \brief Armored fighting vehicle / mechanized combat walker ('R') for
  /// planetary ground warfare.
  OTYPE_AFV,
  /// \brief Fortified military command bunker ('b') protecting troops and
  /// leadership from bombardment.
  OTYPE_BUNKER,
  /// \brief Surface assault transport and planetary landing craft ('L').
  STYPE_LANDER,
};

export inline constexpr int NUMSTYPES = (ShipType::STYPE_LANDER + 1);

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
  UniverseCoordinates coordinates{};  ///< Continuous universe coordinates
  double fuel{0.0};                   ///< Current stored fuel
  double mass{0.0};                   ///< Current total mass
  Coordinates land_coords{0, 0};  ///< Planetary surface coordinates when landed

  shipnum_t destshipno{0};  ///< Destination / escorted ship number

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

  resource_t destruct{0};  ///< Current carried destructive charges
  resource_t resource{0};  ///< Current carried resource cargo
  population_t popn{0};    ///< Current carried colonists / crew
  population_t troops{0};  ///< Current carried military troops
  crystal_t crystals{0};   ///< Current carried warp crystal charge

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
  GunBattery primary_battery;               ///< Primary gun battery
  GunBattery secondary_battery;             ///< Secondary gun battery

  hangar_t hanger{0};      ///< Current docked fighters / payload count
  hangar_t max_hanger{0};  ///< Maximum hangar capacity
};

/// \brief Strongly-typed immutable specifications and capabilities for a ship
/// class.
export struct ShipTemplate {
  ShipType type{ShipType::STYPE_POD};
  std::string_view name;
  char letter{'p'};

  // Baseline capacities & numerical metrics
  double base_tech{0.0};       ///< Baseline technology requirement
  resource_t max_resource{0};  ///< Maximum resource cargo capacity
  hangar_t max_hangar{0};      ///< Maximum hangar capacity
  resource_t max_destruct{0};  ///< Maximum destruct crystal capacity
  gun_count_t max_guns{0};     ///< Number of gun mounts
  guntype_t max_primary_caliber{
      guntype_t::NONE};  ///< Maximum primary gun caliber
  guntype_t max_secondary_caliber{
      guntype_t::NONE};           ///< Maximum secondary gun caliber
  fuel_t max_fuel{0.0};           ///< Maximum fuel tank capacity
  population_t max_crew{0};       ///< Maximum crew accommodation capacity
  armor_t base_armor{0};          ///< Baseline hull armor rating
  money_t build_cost{0};          ///< Base construction cost in currency
  speed_t base_speed{0};          ///< Base engine throttle speed rating
  damage_t base_damage{0};        ///< Base structural damage threshold
  double build_time{0.0};         ///< Construction build time factor
  double construction_cost{0.0};  ///< Construction cost multiplier
  bool can_modify{false};         ///< Can be customized / modified

  // Boolean capabilities & operational permissions
  bool can_mount_laser{false};  ///< Can be equipped with combat laser mount
  bool can_mount{false};        ///< Can mount warp crystals for hyperjump
  bool can_hyperjump{false};    ///< Equipped with hyperjump drive
  bool can_land{false};         ///< Capable of planetary landing
  bool has_switch{false};       ///< Has toggleable power/mode switch
  bool has_cew{false};          ///< Equipped with Concentrated Energy Weapon
  bool can_cloak{false};        ///< Equipped with cloaking device
  bool is_god_only{false};      ///< Restricted to deity/admin creation
  bool is_programmed{false};    ///< Autonomous / automated AI control
  bool is_starport{false};      ///< Operates as a starport
  bool can_repair{false};       ///< Capable of self/fleet repair
  bool requires_maintenance{
      false};  ///< Incurs regular economic maintenance cost

  /// \brief Indicates whether the ship class supports mounting a primary
  /// battery.
  [[nodiscard]] constexpr bool has_primary() const noexcept {
    return max_primary_caliber != guntype_t::NONE;
  }

  /// \brief Indicates whether the ship class supports mounting a secondary
  /// battery.
  [[nodiscard]] constexpr bool has_secondary() const noexcept {
    return max_secondary_caliber != guntype_t::NONE;
  }

  /// \brief Returns whether this ship type can be built on a planetary surface.
  [[nodiscard]] constexpr bool can_build_on_planet() const noexcept {
    return (static_cast<int>(build_time) & 1) != 0;
  }

  /// \brief Returns whether this ship type can be constructed by the specified
  /// builder ship template.
  [[nodiscard]] constexpr bool
  can_be_built_by(const ShipTemplate& builder) const noexcept {
    return (static_cast<int>(build_time) &
            static_cast<int>(builder.construction_cost)) != 0;
  }

  /// \brief Returns whether this ship type is equipped to construct other
  /// ships.
  [[nodiscard]] constexpr bool can_construct_ships() const noexcept {
    return static_cast<int>(construction_cost) != 0;
  }
};

export inline constexpr std::array<ShipTemplate, NUMSTYPES> ship_templates = {{
    // 0: STYPE_POD (Spore pod, 'p')
    {.type = ShipType::STYPE_POD,
     .name = "Spore pod",
     .letter = 'p',
     .base_tech = 0,
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 1,
     .base_armor = 0,
     .build_cost = 1,
     .base_speed = 2,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 25,
     .max_hangar = 2,
     .max_destruct = 2,
     .max_guns = 1,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 10,
     .base_armor = 0,
     .build_cost = 2,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 4,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 600,
     .max_hangar = 200,
     .max_destruct = 800,
     .max_guns = 30,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 1000,
     .max_crew = 30,
     .base_armor = 5,
     .build_cost = 30,
     .base_speed = 4,
     .base_damage = 50,
     .build_time = 20,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 500,
     .max_hangar = 10,
     .max_destruct = 500,
     .max_guns = 60,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 500,
     .max_crew = 60,
     .base_armor = 10,
     .build_cost = 40,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 235,
     .max_hangar = 10,
     .max_destruct = 400,
     .max_guns = 30,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 200,
     .max_crew = 30,
     .base_armor = 7,
     .build_cost = 20,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 110,
     .max_hangar = 5,
     .max_destruct = 120,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::MEDIUM,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 200,
     .max_crew = 20,
     .base_armor = 3,
     .build_cost = 15,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 165,
     .max_hangar = 5,
     .max_destruct = 300,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 120,
     .max_crew = 20,
     .base_armor = 5,
     .build_cost = 10,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 110,
     .max_hangar = 5,
     .max_destruct = 120,
     .max_guns = 15,
     .max_primary_caliber = guntype_t::MEDIUM,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 80,
     .max_crew = 15,
     .base_armor = 3,
     .build_cost = 5,
     .base_speed = 6,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 40,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::MEDIUM,
     .max_secondary_caliber = guntype_t::LIGHT,
     .max_fuel = 10,
     .max_crew = 1,
     .base_armor = 2,
     .build_cost = 1,
     .base_speed = 9,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 10,
     .max_hangar = 0,
     .max_destruct = 15,
     .max_guns = 5,
     .max_primary_caliber = guntype_t::MEDIUM,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 35,
     .max_crew = 5,
     .base_armor = 1,
     .build_cost = 2,
     .base_speed = 6,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 5000,
     .max_hangar = 10,
     .max_destruct = 500,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::MEDIUM,
     .max_secondary_caliber = guntype_t::LIGHT,
     .max_fuel = 2000,
     .max_crew = 2000,
     .base_armor = 3,
     .build_cost = 50,
     .base_speed = 4,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 18,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 5000,
     .max_hangar = 10,
     .max_destruct = 250,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::MEDIUM,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 2000,
     .max_crew = 50,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 4,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 6,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 1400,
     .max_hangar = 20,
     .max_destruct = 1000,
     .max_guns = 50,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 2000,
     .max_crew = 200,
     .base_armor = 5,
     .build_cost = 40,
     .base_speed = 4,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 6,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 1000,
     .max_hangar = 5,
     .max_destruct = 1000,
     .max_guns = 10,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1000,
     .max_crew = 100,
     .base_armor = 2,
     .build_cost = 10,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 4,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 200,
     .max_hangar = 5,
     .max_destruct = 200,
     .max_guns = 10,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 5000,
     .max_crew = 10,
     .base_armor = 2,
     .build_cost = 10,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 2,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 20000,
     .max_hangar = 1000,
     .max_destruct = 20000,
     .max_guns = 1000,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::HEAVY,
     .max_fuel = 20000,
     .max_crew = 1000,
     .base_armor = 100,
     .build_cost = 10,
     .base_speed = 9,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 6,
     .can_modify = true,
     .can_mount_laser = true,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 25,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 0,
     .base_armor = 1,
     .build_cost = 30,
     .base_speed = 2,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 200,
     .max_hangar = 0,
     .max_destruct = 10,
     .max_guns = 1,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 5,
     .base_armor = 0,
     .build_cost = 100,
     .base_speed = 2,
     .base_damage = 75,
     .build_time = 20,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 2,
     .base_armor = 0,
     .build_cost = 20,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 2,
     .base_armor = 0,
     .build_cost = 2,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1000,
     .max_crew = 5,
     .base_armor = 0,
     .build_cost = 20,
     .base_speed = 2,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 200,
     .max_crew = 10,
     .base_armor = 1,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 10,
     .base_speed = 1,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 10,
     .base_speed = 1,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 20,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 50,
     .max_crew = 0,
     .base_armor = 1,
     .build_cost = 100,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 50,
     .max_hangar = 0,
     .max_destruct = 500,
     .max_guns = 40,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::MEDIUM,
     .max_fuel = 1000,
     .max_crew = 0,
     .base_armor = 15,
     .build_cost = 100,
     .base_speed = 6,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 500,
     .max_hangar = 0,
     .max_destruct = 100,
     .max_guns = 10,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1000,
     .max_crew = 10,
     .base_armor = 20,
     .build_cost = 500,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 25,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 100,
     .max_crew = 2,
     .base_armor = 1,
     .build_cost = 50,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 5,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 10,
     .base_speed = 9,
     .base_damage = 0,
     .build_time = 19,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 50,
     .max_hangar = 0,
     .max_destruct = 120,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 40,
     .base_armor = 3,
     .build_cost = 30,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 50,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 20,
     .base_armor = 0,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 17,
     .construction_cost = 8,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 40,
     .max_hangar = 5,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 200,
     .max_crew = 20,
     .base_armor = 1,
     .build_cost = 20,
     .base_speed = 4,
     .base_damage = 0,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 200,
     .max_hangar = 0,
     .max_destruct = 50,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 0,
     .base_armor = 10,
     .build_cost = 3,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 1000,
     .max_hangar = 0,
     .max_destruct = 1000,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1000,
     .max_crew = 0,
     .base_armor = 10,
     .build_cost = 8,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 1000,
     .max_hangar = 0,
     .max_destruct = 1000,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 1000,
     .max_crew = 100,
     .base_armor = 0,
     .build_cost = 300,
     .base_speed = 0,
     .base_damage = 50,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 10,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 5,
     .max_crew = 0,
     .base_armor = 0,
     .build_cost = 5,
     .base_speed = 6,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 50,
     .max_hangar = 0,
     .max_destruct = 500,
     .max_guns = 20,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 50,
     .base_armor = 10,
     .build_cost = 100,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 200,
     .max_crew = 50,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 200,
     .max_crew = 10,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 100,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 20,
     .base_armor = 1,
     .build_cost = 10,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 500,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 500,
     .max_crew = 20,
     .base_armor = 5,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 17,
     .construction_cost = 0,
     .can_modify = false,
     .can_mount_laser = false,
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
     .max_resource = 0,
     .max_hangar = 0,
     .max_destruct = 0,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 100,
     .base_armor = 3,
     .build_cost = 50,
     .base_speed = 0,
     .base_damage = 75,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 5,
     .max_hangar = 0,
     .max_destruct = 50,
     .max_guns = 5,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 0,
     .max_crew = 5,
     .base_armor = 5,
     .build_cost = 50,
     .base_speed = 0,
     .base_damage = 50,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 5,
     .max_hangar = 0,
     .max_destruct = 20,
     .max_guns = 2,
     .max_primary_caliber = guntype_t::LIGHT,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 20,
     .max_crew = 1,
     .base_armor = 2,
     .build_cost = 20,
     .base_speed = 0,
     .base_damage = 0,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 100,
     .max_hangar = 20,
     .max_destruct = 100,
     .max_guns = 0,
     .max_primary_caliber = guntype_t::NONE,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 100,
     .max_crew = 100,
     .base_armor = 15,
     .build_cost = 100,
     .base_speed = 0,
     .base_damage = 50,
     .build_time = 1,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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
     .max_resource = 100,
     .max_hangar = 10,
     .max_destruct = 200,
     .max_guns = 10,
     .max_primary_caliber = guntype_t::HEAVY,
     .max_secondary_caliber = guntype_t::NONE,
     .max_fuel = 100,
     .max_crew = 500,
     .base_armor = 7,
     .build_cost = 50,
     .base_speed = 2,
     .base_damage = 50,
     .build_time = 8,
     .construction_cost = 0,
     .can_modify = true,
     .can_mount_laser = false,
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

/// \brief Returns an array containing the classification letters of all ship
/// types.
export [[nodiscard]] constexpr std::array<char, NUMSTYPES>
get_all_ship_letters() noexcept {
  std::array<char, NUMSTYPES> letters{};
  for (std::size_t i = 0; i < NUMSTYPES; ++i) {
    letters[i] = ship_templates[i].letter;
  }
  return letters;
}

/// \brief Returns whether the given character corresponds to a known ship
/// classification letter.
export [[nodiscard]] constexpr bool is_valid_ship_letter(char c) noexcept {
  return std::ranges::any_of(
      ship_templates, [c](const ShipTemplate& t) { return t.letter == c; });
}

/// \brief Type-safe accessor for primary gun caliber from ShipTemplate.
/// \param ship_type The ship type to query.
/// \return Primary gun caliber as guntype_t.
export [[nodiscard]] constexpr guntype_t
shipdata_primary(ShipType ship_type) noexcept {
  return ship_template(ship_type).max_primary_caliber;
}

/// \brief Type-safe accessor for secondary gun caliber from ShipTemplate.
/// \param ship_type The ship type to query.
/// \return Secondary gun caliber as guntype_t.
export [[nodiscard]] constexpr guntype_t
shipdata_secondary(ShipType ship_type) noexcept {
  return ship_template(ship_type).max_secondary_caliber;
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

  /// \brief Returns whether this ship is an in-memory simulation clone.
  [[nodiscard]] virtual bool is_simulation() const noexcept {
    return false;
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
  /// \brief Returns continuous position in universe coordinates.
  [[nodiscard]] constexpr UniverseCoordinates coordinates() const noexcept {
    return data_.coordinates;
  }

  /// \brief Sets continuous position in universe coordinates.
  constexpr void set_coordinates(UniverseCoordinates coords) noexcept {
    data_.coordinates = coords;
  }

  // Resources
  [[nodiscard]] double fuel() const noexcept {
    return data_.fuel;
  }

  [[nodiscard]] double mass() const noexcept {
    return data_.mass;
  }
  void set_mass(double mass) noexcept {
    data_.mass = mass;
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

  /// \brief Calculates empty hull baseline mass based on armor, size, hangar,
  /// and gun batteries.
  [[nodiscard]] double base_mass() const noexcept;

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

  [[nodiscard]] crystal_t crystals() const noexcept {
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
  [[nodiscard]] damage_t damage() const noexcept {
    return data_.damage;
  }

  [[nodiscard]] radiation_t rad() const noexcept {
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

  /// \brief Indicates whether this ship class can be equipped with a combat
  /// laser.
  [[nodiscard]] constexpr bool can_mount_laser() const noexcept {
    return get_template().can_mount_laser;
  }

  /// \brief Indicates whether this ship class operates as a space port.
  [[nodiscard]] constexpr bool is_starport() const noexcept {
    return get_template().is_starport;
  }

  /// \brief Indicates whether this ship class is equipped to construct other
  /// ships.
  [[nodiscard]] constexpr bool can_construct_ships() const noexcept {
    return get_template().can_construct_ships();
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

  // Gun Batteries
  [[nodiscard]] const GunBattery& primary_battery() const noexcept {
    return data_.primary_battery;
  }
  [[nodiscard]] const GunBattery& secondary_battery() const noexcept {
    return data_.secondary_battery;
  }

  /// \brief Returns a pointer to the active gun battery mount, or nullptr if
  /// gun battery mode is NONE or the selected battery has no guns mounted.
  [[nodiscard]] const GunBattery* active_gun_battery() const noexcept {
    if (data_.guns == ActiveBattery::PRIMARY &&
        data_.primary_battery.has_guns()) {
      return &data_.primary_battery;
    }
    if (data_.guns == ActiveBattery::SECONDARY &&
        data_.secondary_battery.has_guns()) {
      return &data_.secondary_battery;
    }
    return nullptr;
  }

  /// \brief Caliber of the active gun battery, or guntype_t::NONE if
  /// offline/empty.
  [[nodiscard]] guntype_t active_gun_caliber() const noexcept {
    const auto* battery = active_gun_battery();
    return battery ? battery->caliber : guntype_t::NONE;
  }

  /// \brief Formats a summary of both batteries, e.g. "10H/5L" or "0 /0 ".
  [[nodiscard]] std::string battery_summary() const {
    return std::format("{}/{}", primary_battery().to_string(),
                       secondary_battery().to_string());
  }

  /// \brief Atomically sets the primary battery count and caliber.
  void set_primary_battery(gun_count_t count, guntype_t caliber) noexcept {
    data_.primary_battery.set(count, caliber);
  }

  /// \brief Atomically sets the primary battery from a GunBattery value object.
  void set_primary_battery(GunBattery battery) noexcept {
    data_.primary_battery.set(battery.count, battery.caliber);
  }

  /// \brief Atomically sets the secondary battery count and caliber.
  void set_secondary_battery(gun_count_t count, guntype_t caliber) noexcept {
    data_.secondary_battery.set(count, caliber);
  }

  /// \brief Atomically sets the secondary battery from a GunBattery value
  /// object.
  void set_secondary_battery(GunBattery battery) noexcept {
    data_.secondary_battery.set(battery.count, battery.caliber);
  }

  /// \brief Applies collateral damage to the primary gun battery mounts.
  /// \return The actual number of primary guns destroyed.
  gun_count_t damage_primary_guns(gun_count_t hits) noexcept {
    return data_.primary_battery.damage(hits);
  }

  /// \brief Applies collateral damage to the secondary gun battery mounts.
  /// \return The actual number of secondary guns destroyed.
  gun_count_t damage_secondary_guns(gun_count_t hits) noexcept {
    return data_.secondary_battery.damage(hits);
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
    return get_template().has_switch;
  }

  /// Whether ship has planetary bombardment weapon capabilities.
  [[nodiscard]] bool can_bombard() const noexcept {
    return get_template().max_guns != 0 && (data_.type != ShipType::STYPE_MINE);
  }

  /// Whether ship is capable of independent orbital navigation.
  [[nodiscard]] bool can_navigate() const noexcept {
    return get_template().base_speed > 0 &&
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
               ? get_template().base_armor
               : static_cast<armor_t>(data_.armor * (100 - data_.damage) / 100);
  }

  /// Active weapon battery strength based on selected gun mode.
  [[nodiscard]] gun_count_t active_guns() const noexcept {
    const auto* battery = active_gun_battery();
    return battery ? battery->count : 0;
  }

  /// Structural body size excluding maximum hangar bay space.
  [[nodiscard]] ship_size_t shipbody() const noexcept {
    return data_.size > data_.max_hanger ? data_.size - data_.max_hanger : 0;
  }

  /// Remaining available hangar space for docking smaller craft.
  [[nodiscard]] hangar_t hanger_space() const noexcept {
    return data_.max_hanger > data_.hanger ? data_.max_hanger - data_.hanger
                                           : 0;
  }

  /// Available civilian crew capacity accounting for military troops on board.
  [[nodiscard]] population_t available_crew() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<population_t>(get_template().max_crew -
                                           data_.troops)
               : (data_.max_crew - data_.troops);
  }

  /// Available military troop capacity accounting for civilian crew on board.
  [[nodiscard]] population_t available_mil() const noexcept {
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? static_cast<population_t>(get_template().max_crew - data_.popn)
               : (data_.max_crew - data_.popn);
  }

  /// \brief Available berth capacity for crew and troops combined
  /// (max_crew_capacity() - (popn + troops)).
  [[nodiscard]] population_t available_crew_capacity() const noexcept {
    const auto total = data_.popn + data_.troops;
    const auto max_cap = max_crew_capacity();
    return (total >= max_cap) ? 0 : (max_cap - total);
  }

  /// \brief Epsilon threshold for fuel comparisons and consumption tests.
  static constexpr double FUEL_EPSILON = 1e-4;

  /// Maximum total crew capacity including factory template overrides.
  [[nodiscard]] population_t max_crew_capacity() const noexcept {
    if (data_.max_crew > 0) return data_.max_crew;
    return (data_.type == ShipType::OTYPE_FACTORY) ? get_template().max_crew
                                                   : data_.max_crew;
  }

  /// Maximum cargo resource capacity including factory template overrides.
  [[nodiscard]] resource_t max_resource_capacity() const noexcept {
    if (data_.max_resource > 0) return data_.max_resource;
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? ship_template(data_.type).max_resource
               : data_.max_resource;
  }

  /// Maximum fuel tank capacity including factory template overrides.
  [[nodiscard]] fuel_t max_fuel_capacity() const noexcept {
    if (data_.max_fuel > 0.0) return data_.max_fuel;
    return (data_.type == ShipType::OTYPE_FACTORY)
               ? ship_template(data_.type).max_fuel
               : data_.max_fuel;
  }

  /// Maximum ammo / ordnance capacity including factory template overrides.
  [[nodiscard]] resource_t max_destruct_capacity() const noexcept {
    if (data_.max_destruct > 0) return data_.max_destruct;
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
  [[nodiscard]] crystal_t max_crystals_capacity() const noexcept {
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
               ? 2L * data_.build_cost * data_.on + get_template().build_cost
               : data_.build_cost;
  }

  /// Ship classification type letter code.
  [[nodiscard]] char type_letter() const noexcept {
    return get_template().letter;
  }

  /// Ship classification type name.
  [[nodiscard]] std::string_view type_name() const noexcept {
    return get_template().name;
  }

  /// Maximum gun mount capacity from ship template.
  [[nodiscard]] gun_count_t max_guns_capacity() const noexcept {
    return get_template().max_guns;
  }

  /// \brief Hull operational efficiency in [0.0, 1.0] based on damage (1.0 at
  /// 0% damage, 0.0 at 100% damage).
  [[nodiscard]] constexpr double hull_efficiency() const noexcept {
    return std::clamp((100.0 - static_cast<double>(data_.damage)) / 100.0, 0.0,
                      1.0);
  }

  /// \brief Ratio of carried crew to maximum capacity in [0.0, 1.0] (0.0 if
  /// ship has zero capacity).
  [[nodiscard]] double crew_ratio() const noexcept {
    const auto max_crew = max_crew_capacity();
    if (max_crew <= 0) return 0.0;
    return std::clamp(static_cast<double>(data_.popn) /
                          static_cast<double>(max_crew),
                      0.0, 1.0);
  }

  /// \brief Returns whether ship is fueled to maximum capacity (within
  /// epsilon).
  [[nodiscard]] bool is_fully_fueled() const noexcept {
    const auto max_fuel = max_fuel_capacity();
    if (max_fuel == 0) return false;
    return data_.fuel >= static_cast<double>(max_fuel) - FUEL_EPSILON;
  }

  /// \brief Returns whether ship has non-negligible fuel remaining.
  [[nodiscard]] bool has_fuel() const noexcept {
    return data_.fuel > FUEL_EPSILON;
  }

  /// \brief Available cargo capacity remaining for resources.
  [[nodiscard]] resource_t available_resource_capacity() const noexcept {
    return std::max<resource_t>(0, max_resource_capacity() - data_.resource);
  }

  // =========================================================================
  // DOMAIN OPERATIONS & STATE TRANSITIONS
  // =========================================================================

  // =========================================================================
  // ADMIN / DEITY OVERRIDES (Commands: fix, do_god)
  // =========================================================================

  /// \brief Deity override: sets hull damage clamped to [0, 100]%.
  void admin_override_damage(damage_t amt) noexcept {
    data_.damage = std::min<damage_t>(amt, 100);
  }

  /// \brief Deity override: sets hull damage clamped to [0, 100]%, guarding
  /// against negative values.
  template <std::signed_integral T>
  void admin_override_damage(T amt) noexcept {
    data_.damage = static_cast<damage_t>(
        std::clamp<std::int64_t>(static_cast<std::int64_t>(amt), 0, 100));
  }

  /// \brief Deity override: sets radiation dosage clamped to [0, 100]%.
  void admin_override_radiation(radiation_t amt) noexcept {
    data_.rad = std::min<radiation_t>(amt, 100);
  }

  /// \brief Deity override: sets radiation dosage clamped to [0, 100]%,
  /// guarding against negative values.
  template <std::signed_integral T>
  void admin_override_radiation(T amt) noexcept {
    data_.rad = static_cast<radiation_t>(
        std::clamp<std::int64_t>(static_cast<std::int64_t>(amt), 0, 100));
  }

  /// \brief Deity override: sets fuel clamped to capacity and synchronizes
  /// mass.
  void admin_override_fuel(fuel_t amt, double race_mass = 1.0) noexcept {
    data_.fuel = std::clamp(amt, 0.0, static_cast<double>(max_fuel_capacity()));
    data_.mass = local_mass(race_mass);
  }

  /// \brief Deity override: sets cargo resources clamped to capacity and
  /// synchronizes mass.
  void admin_override_resource(resource_t amt,
                               double race_mass = 1.0) noexcept {
    data_.resource = std::clamp<resource_t>(amt, 0, max_resource_capacity());
    data_.mass = local_mass(race_mass);
  }

  /// \brief Deity override: sets destruct charges clamped to capacity and
  /// synchronizes mass.
  void admin_override_destruct(resource_t amt,
                               double race_mass = 1.0) noexcept {
    data_.destruct = std::clamp<resource_t>(amt, 0, max_destruct_capacity());
    data_.mass = local_mass(race_mass);
  }

  /// \brief Deity override: sets warp crystal charges clamped to capacity.
  void admin_override_crystals(crystal_t count) noexcept {
    data_.crystals = std::min(count, max_crystals_capacity());
  }

  /// \brief Deity override: sets maximum fuel tank capacity.
  void admin_override_max_fuel(fuel_t amt) noexcept {
    data_.max_fuel = std::max(0.0, amt);
  }

  /// \brief Deity override: resurrects a destroyed ship to full operational
  /// health.
  void admin_resurrect() noexcept {
    data_.alive = true;
    data_.active = true;
    data_.damage = 0;
  }

  /// \brief Deity override: marks a ship as destroyed.
  void admin_destroy() noexcept {
    data_.alive = false;
    data_.active = false;
    data_.damage = 100;
  }

  /// \brief Atomically eliminates all crew and troops (surrender/boarding) and
  /// decrements biological mass.
  void clear_crew(double race_mass = 1.0) noexcept {
    const auto lost_crew = data_.popn + data_.troops;
    data_.popn = 0;
    data_.troops = 0;
    data_.mass -= static_cast<double>(lost_crew) * race_mass;
    if (data_.mass < base_mass()) data_.mass = base_mass();
  }

  /// \brief Consumes crystals clamped to available stock; returns actual
  /// consumed.
  crystal_t consume_crystals(std::int64_t amt) noexcept {
    if (amt <= 0) return 0;
    const auto actual =
        std::min<crystal_t>(data_.crystals, static_cast<crystal_t>(amt));
    data_.crystals -= actual;
    return actual;
  }

  /// \brief Adds crystals clamped to max capacity. Negative values delegate to
  /// consume_crystals(-amt).
  void add_crystals(std::int64_t amt) noexcept {
    if (amt < 0) {
      consume_crystals(-amt);
      return;
    }
    const auto max_cap = max_crystals_capacity();
    if (data_.crystals >= max_cap) return;
    const auto actual = std::min<crystal_t>(static_cast<crystal_t>(amt),
                                            max_cap - data_.crystals);
    data_.crystals += actual;
  }

  /// \brief Authoritative single-ship intrinsic physical mass calculation.
  [[nodiscard]] double local_mass(double race_mass = 1.0) const noexcept;

  /// \brief Attempts to consume an exact amount of fuel; returns true on
  /// success, false if insufficient fuel.
  [[nodiscard]] bool try_consume_fuel(fuel_t cost) noexcept {
    if (cost <= 0.0) return true;
    if (data_.fuel + FUEL_EPSILON < cost) return false;
    const auto actual = std::min(data_.fuel, cost);
    data_.fuel -= actual;
    if (data_.fuel < 0.0) data_.fuel = 0.0;
    data_.mass -= actual * MASS_FUEL;
    return true;
  }

  /// \brief Consumes up to the requested amount of fuel, returning the amount
  /// actually consumed.
  [[nodiscard]] fuel_t consume_up_to_fuel(fuel_t max_amount) noexcept {
    if (max_amount <= 0.0) return 0.0;
    const auto actual = std::min(data_.fuel, max_amount);
    data_.fuel -= actual;
    if (data_.fuel < 0.0) data_.fuel = 0.0;
    data_.mass -= actual * MASS_FUEL;
    return actual;
  }

  /// \brief Attempts to consume an exact amount of resource cargo; returns
  /// true on success, false if insufficient resources.
  [[nodiscard]] bool try_consume_resource(resource_t cost) noexcept {
    if (cost <= 0) return true;
    if (data_.resource < cost) return false;
    data_.resource -= cost;
    data_.mass -= static_cast<double>(cost) * MASS_RESOURCE;
    return true;
  }

  /// \brief Consumes up to the requested amount of resource cargo, returning
  /// the amount actually consumed.
  [[nodiscard]] resource_t
  consume_up_to_resource(resource_t max_amount) noexcept {
    if (max_amount <= 0) return 0;
    const auto actual = std::min(data_.resource, max_amount);
    data_.resource -= actual;
    data_.mass -= static_cast<double>(actual) * MASS_RESOURCE;
    return actual;
  }

  /// \brief Attempts to consume an exact amount of destructive charges; returns
  /// true on success, false if insufficient charges.
  [[nodiscard]] bool try_consume_destruct(resource_t cost) noexcept {
    if (cost <= 0) return true;
    if (data_.destruct < cost) return false;
    data_.destruct -= cost;
    data_.mass -= static_cast<double>(cost) * MASS_DESTRUCT;
    return true;
  }

  /// \brief Consumes up to the requested amount of destructive charges,
  /// returning the amount actually consumed.
  [[nodiscard]] resource_t
  consume_up_to_destruct(resource_t max_amount) noexcept {
    if (max_amount <= 0) return 0;
    const auto actual = std::min(data_.destruct, max_amount);
    data_.destruct -= actual;
    data_.mass -= static_cast<double>(actual) * MASS_DESTRUCT;
    return actual;
  }

  /// \brief Increases hull damage by the specified amount, clamped to 100%.
  ///
  /// Protected against unsigned overflow and underflow. Returns a DamageResult
  /// indicating actual damage added, new damage level, and whether the ship was
  /// destroyed.
  [[nodiscard]] DamageResult apply_damage(damage_t amt) noexcept {
    if (data_.damage >= 100) {
      data_.damage = 100;
      return {
          .damage_applied = 0,
          .new_damage = 100,
          .destroyed = true,
      };
    }
    const damage_t prev = data_.damage;
    if (amt >= 100 || 100 - data_.damage <= amt) {
      data_.damage = 100;
    } else {
      data_.damage += amt;
    }
    return {
        .damage_applied = data_.damage - prev,
        .new_damage = data_.damage,
        .destroyed = (data_.damage >= 100),
    };
  }

  /// \brief Increases hull damage by a signed amount, guarding against negative
  /// values.
  template <std::signed_integral T>
  [[nodiscard]] DamageResult apply_damage(T amt) noexcept {
    if (amt <= 0) {
      return {
          .damage_applied = 0,
          .new_damage = data_.damage,
          .destroyed = (data_.damage >= 100),
      };
    }
    return apply_damage(static_cast<damage_t>(amt));
  }

  /// \brief Increases hull damage by a floating-point amount, rounding and
  /// guarding against negative values.
  template <std::floating_point T>
  [[nodiscard]] DamageResult apply_damage(T amt) noexcept {
    if (amt <= 0.0) {
      return {
          .damage_applied = 0,
          .new_damage = data_.damage,
          .destroyed = (data_.damage >= 100),
      };
    }
    return apply_damage(static_cast<damage_t>(std::round(amt)));
  }

  /// \brief Repairs hull damage by the specified amount, clamped to 0%.
  void repair_damage(damage_t amt) noexcept {
    data_.damage = (amt >= data_.damage) ? 0 : data_.damage - amt;
  }

  /// \brief Repairs hull damage by a signed amount, guarding against negative
  /// values.
  template <std::signed_integral T>
  void repair_damage(T amt) noexcept {
    if (amt <= 0) return;
    repair_damage(static_cast<damage_t>(amt));
  }

  /// \brief Applies radiation dosage following peak-dose semantics, clamped to
  /// [0, 100]%.
  void apply_radiation(radiation_t dosage) noexcept {
    const auto clamped_dose = std::min<radiation_t>(dosage, 100);
    data_.rad = std::max(data_.rad, clamped_dose);
  }

  /// \brief Applies radiation dosage by a signed amount, guarding against
  /// negative values.
  template <std::signed_integral T>
  void apply_radiation(T dosage) noexcept {
    if (dosage <= 0) return;
    apply_radiation(static_cast<radiation_t>(dosage));
  }

  /// \brief Reduces accumulated radiation dose, clamped to 0.
  void repair_radiation(radiation_t amt) noexcept {
    data_.rad = (amt >= data_.rad) ? 0 : data_.rad - amt;
  }

  /// \brief Reduces accumulated radiation dose by a signed amount, guarding
  /// against negative values.
  template <std::signed_integral T>
  void repair_radiation(T amt) noexcept {
    if (amt <= 0) return;
    repair_radiation(static_cast<radiation_t>(amt));
  }

  /// \brief Consumes fuel and decrements ship mass accordingly, clamped to 0.
  void consume_fuel(fuel_t amt) noexcept {
    if (amt <= 0.0) return;
    const auto actual = std::min(data_.fuel, amt);
    data_.fuel -= actual;
    if (data_.fuel < 0.0) data_.fuel = 0.0;
    data_.mass -= actual * MASS_FUEL;
  }

  /// \brief Adds fuel and increments ship mass accordingly, clamped to max fuel
  /// capacity. If amt is negative, delegates to consume_fuel(-amt).
  void add_fuel(fuel_t amt) noexcept {
    if (amt < 0.0) {
      consume_fuel(-amt);
      return;
    }
    const auto max_cap = static_cast<double>(max_fuel_capacity());
    if (data_.fuel >= max_cap) return;
    const auto actual = std::min(amt, max_cap - data_.fuel);
    data_.fuel += actual;
    data_.mass += actual * MASS_FUEL;
  }

  /// \brief Consumes resources and decrements ship mass accordingly, clamped
  /// to 0.
  void consume_resource(resource_t amt) noexcept {
    if (amt <= 0) return;
    const auto actual = std::min(data_.resource, amt);
    data_.resource -= actual;
    data_.mass -= static_cast<double>(actual) * MASS_RESOURCE;
  }

  /// \brief Adds resources and increments ship mass accordingly, clamped to
  /// max resource capacity. If amt is negative, delegates to
  /// consume_resource(-amt).
  void add_resource(resource_t amt) noexcept {
    if (amt < 0) {
      consume_resource(-amt);
      return;
    }
    const auto max_cap = max_resource_capacity();
    if (data_.resource >= max_cap) return;
    const auto actual = std::min(amt, max_cap - data_.resource);
    data_.resource += actual;
    data_.mass += static_cast<double>(actual) * MASS_RESOURCE;
  }

  /// \brief Consumes destruct ordnance and decrements ship mass accordingly,
  /// clamped to 0.
  void consume_destruct(resource_t amt) noexcept {
    if (amt <= 0) return;
    const auto actual = std::min(data_.destruct, amt);
    data_.destruct -= actual;
    data_.mass -= static_cast<double>(actual) * MASS_DESTRUCT;
  }

  /// \brief Adds destruct ordnance and increments ship mass accordingly,
  /// clamped to max destruct capacity. If amt is negative, delegates to
  /// consume_destruct(-amt).
  void add_destruct(resource_t amt) noexcept {
    if (amt < 0) {
      consume_destruct(-amt);
      return;
    }
    const auto max_cap = max_destruct_capacity();
    if (data_.destruct >= max_cap) return;
    const auto actual = std::min(amt, max_cap - data_.destruct);
    data_.destruct += actual;
    data_.mass += static_cast<double>(actual) * MASS_DESTRUCT;
  }

  /// \brief Adds population and increments ship mass based on race mass,
  /// clamped to available joint crew capacity (popn + troops <=
  /// max_crew_capacity()). If amt is negative, delegates to remove_popn(-amt,
  /// race_mass).
  void add_popn(population_t amt, double race_mass) noexcept {
    if (amt < 0) {
      remove_popn(-amt, race_mass);
      return;
    }
    const auto avail = available_crew_capacity();
    if (avail == 0) return;
    const auto actual = std::min(amt, avail);
    data_.popn += actual;
    data_.mass += static_cast<double>(actual) * race_mass;
  }

  /// \brief Removes population and decrements ship mass based on race mass,
  /// clamped to 0.
  void remove_popn(population_t amt, double race_mass) noexcept {
    if (amt <= 0) return;
    const auto actual = std::min(data_.popn, amt);
    data_.popn -= actual;
    data_.mass -= static_cast<double>(actual) * race_mass;
  }

  /// \brief Adds troops and increments ship mass based on race mass,
  /// clamped to available joint crew capacity (popn + troops <=
  /// max_crew_capacity()). If amt is negative, delegates to remove_troops(-amt,
  /// race_mass).
  void add_troops(population_t amt, double race_mass) noexcept {
    if (amt < 0) {
      remove_troops(-amt, race_mass);
      return;
    }
    const auto avail = available_crew_capacity();
    if (avail == 0) return;
    const auto actual = std::min(amt, avail);
    data_.troops += actual;
    data_.mass += static_cast<double>(actual) * race_mass;
  }

  /// \brief Removes troops and decrements ship mass based on race mass,
  /// clamped to 0.
  void remove_troops(population_t amt, double race_mass) noexcept {
    if (amt <= 0) return;
    const auto actual = std::min(data_.troops, amt);
    data_.troops -= actual;
    data_.mass -= static_cast<double>(actual) * race_mass;
  }

  /// \brief Casualties inflicted on ship personnel.
  struct Casualties {
    population_t crew{0};
    population_t troops{0};
  };

  /// \brief Inflicts casualties on ship crew and troops, and updates ship mass
  /// accordingly. Neither crew nor troops is reduced below 0.
  /// \return Actual casualties deducted {crew_lost, troops_lost}.
  Casualties apply_casualties(population_t crew_loss, population_t troop_loss,
                              double race_mass = 1.0) noexcept {
    const auto old_popn = data_.popn;
    const auto old_troops = data_.troops;
    remove_popn(crew_loss, race_mass);
    remove_troops(troop_loss, race_mass);
    return {.crew = old_popn - data_.popn, .troops = old_troops - data_.troops};
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

  /// Resolves the absolute coordinates of the aimed target.
  [[nodiscard]] std::optional<UniverseCoordinates>
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

/// \brief Transient in-memory ship clone for "what-if" flight simulations.
/// Guarantees that hypothetical modifications cannot be persisted to the
/// database.
export class SimulatedShip : public Ship {
public:
  explicit SimulatedShip(const Ship& base) : Ship(base.get_struct()) {
    data_.number = 0;  // Neutralize entity identity: cannot match or overwrite
                       // real entities
  }

  [[nodiscard]] bool is_simulation() const noexcept override {
    return true;
  }

  /// \brief Sets simulated fuel and updates mass accordingly, clamped to max
  /// capacity.
  void set_simulated_fuel(fuel_t fuel, double race_mass = 1.0) noexcept {
    const auto max_cap = static_cast<double>(max_fuel_capacity());
    data_.fuel = std::clamp(fuel, 0.0, max_cap);
    data_.mass = local_mass(race_mass);
  }

  /// \brief Sets simulated temporary flight destination and undocks.
  void set_simulated_destination(ScopeLevel level, starnum_t snum,
                                 planetnum_t pnum,
                                 shipnum_t shipno = shipnum_t{0}) noexcept {
    destshipno() = shipno;
    whatdest() = level;
    deststar() = snum;
    destpnum() = pnum;
    docked() = 0;
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
static_assert(sizeof(SimulatedShip) == sizeof(Ship));

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

export template <std::derived_from<Ship> T>
struct std::formatter<T> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const T& s, auto& ctx) const {
    return std::format_to(ctx.out(), "{}{}{} [{}]", s.type_letter(), s.number(),
                          s.name(), s.owner());
  }
};

/// Check if ship type appears in filter string
/// \param type Ship type to check
/// \param filter String containing ship type letters to match
/// \return True if ship type letter appears in filter string
export inline bool listed(ShipType type, std::string_view filter) {
  return std::ranges::any_of(
      filter, [type](char c) { return ship_template(type).letter == c; });
}
