// SPDX-License-Identifier: Apache-2.0

/// \file ship_types.cppm
/// \brief Ship domain enums, component value objects, and serialization POD
/// struct.

export module gb.entities:ship_types;

import std;

import :types;
import :tweakables;

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

/// \brief Parses a gun battery caliber name ("light", "medium", "heavy") into a
/// guntype_t.
export constexpr std::optional<guntype_t>
parse_caliber_name(std::string_view name) noexcept {
  if (name == "light") return guntype_t::LIGHT;
  if (name == "medium") return guntype_t::MEDIUM;
  if (name == "heavy") return guntype_t::HEAVY;
  return std::nullopt;
}

export enum class ActiveBattery : std::uint8_t {
  NONE = 0,
  PRIMARY = 1,
  SECONDARY = 2,
};

/// \brief Physical mooring / surface status of a ship.
export enum class DockState : std::uint8_t {
  Spaceborne = 0,  ///< Orbiting or moving through space
  Landed = 1,      ///< Landed on a planetary surface
  Docked = 2,      ///< Docked inside a carrier ship's hangar
};

/// \brief Commodity and personnel cargo types transferable between ships.
export enum class ShipCargoType : std::uint8_t {
  Resource,
  Destruct,
  Fuel,
  Crystal,
  Crew,
  Troops,
};

/// \brief Converts single-character commodity abbreviation to ShipCargoType.
export constexpr std::optional<ShipCargoType>
char_to_ship_cargo(char c) noexcept {
  switch (c) {
    case 'r':
      return ShipCargoType::Resource;
    case 'd':
      return ShipCargoType::Destruct;
    case 'f':
      return ShipCargoType::Fuel;
    case 'x':
    case '&':
      return ShipCargoType::Crystal;
    case 'c':
      return ShipCargoType::Crew;
    case 'm':
      return ShipCargoType::Troops;
    default:
      return std::nullopt;
  }
}

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
  shipnum_t shipno{0};                      /* aimed at what ship */
  starnum_t snum{0};                        /* aimed at what star */
  int intensity{0};                         /* intensity of aiming */
  planetnum_t pnum{0};                      /* aimed at what planet */
  ScopeLevel level{ScopeLevel::LEVEL_UNIV}; /* aimed at what level */
};

/// Brain parameters for Von Neumann machines and Berserkers.
export struct MindData {
  player_t progenitor{0};  ///< Original race that created this strain
  std::optional<player_t> target{
      std::nullopt};            ///< Target player to destroy (for Berserkers)
  std::uint32_t generation{0};  ///< Reproduction generation counter
  bool busy{false};      ///< Whether machine is currently occupied with a task
  bool tampered{false};  ///< Whether machine brain was reprogrammed by an alien
  std::optional<player_t> who_killed{
      std::nullopt};  ///< Player who destroyed progenitor machine
};

export struct PodData {
  int decay{0};
  int temperature{0};
};

export struct TimerData {
  int count{0};
};

export struct ImpactData {
  Coordinates coords{0, 0};
  bool scatter{false};
};

export struct TriggerData {
  weapon_range_t radius{0};
};

export struct TerraformData {
  int index{0};
};

export struct TransportData {
  shipnum_t target{0};
};

export struct WasteData {
  int toxic{0};
};

// Variant type for special ship functions (std::monostate is index 0 for
// standard hulls that have no specialized payload)
export using SpecialData =
    std::variant<std::monostate, /* standard ships with no special payload */
                 AimedAtData,    /* Space Mirror, telescopes, tractor beams */
                 MindData,       /* VNs and berserkers */
                 PodData,        /* spore pods */
                 TimerData,      /* dust canisters, greenhouse gases */
                 ImpactData,     /* missiles */
                 TriggerData,    /* mines */
                 TerraformData,  /* terraformers, ground plows */
                 TransportData,  /* AVPM transporter */
                 WasteData       /* toxic waste containers */
                 >;

/// Returns the canonical default SpecialData variant alternative for a given
/// ShipType and owner. Intentionally omits a `default:` label so the compiler
/// enforces exhaustive coverage across all ShipType enumerators.
export [[nodiscard]] constexpr SpecialData
default_special_data(ShipType type, player_t owner = 0) noexcept {
  switch (type) {
    case ShipType::OTYPE_VN:
    case ShipType::OTYPE_BERS:
      return MindData{.progenitor = owner,
                      .target = std::nullopt,
                      .generation = 1,
                      .busy = true,
                      .tampered = false,
                      .who_killed = std::nullopt};
    case ShipType::STYPE_MIRROR:
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE:
    case ShipType::OTYPE_TRACT:
      return AimedAtData{};
    case ShipType::STYPE_POD:
      return PodData{};
    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      return TimerData{};
    case ShipType::STYPE_MISSILE:
      return ImpactData{};
    case ShipType::STYPE_MINE:
      return TriggerData{.radius = 100};
    case ShipType::OTYPE_TERRA:
    case ShipType::OTYPE_PLOW:
      return TerraformData{};
    case ShipType::OTYPE_TRANSDEV:
      return TransportData{};
    case ShipType::OTYPE_TOXWC:
      return WasteData{};
    case ShipType::STYPE_SHUTTLE:
    case ShipType::STYPE_CARRIER:
    case ShipType::STYPE_DREADNT:
    case ShipType::STYPE_BATTLE:
    case ShipType::STYPE_INTCPT:
    case ShipType::STYPE_CRUISER:
    case ShipType::STYPE_DESTROYER:
    case ShipType::STYPE_FIGHTER:
    case ShipType::STYPE_EXPLORER:
    case ShipType::STYPE_HABITAT:
    case ShipType::STYPE_STATION:
    case ShipType::STYPE_OAP:
    case ShipType::STYPE_CARGO:
    case ShipType::STYPE_TANKER:
    case ShipType::STYPE_GOD:
    case ShipType::OTYPE_AP:
    case ShipType::OTYPE_GOV:
    case ShipType::OTYPE_OMCL:
    case ShipType::OTYPE_PROBE:
    case ShipType::OTYPE_GR:
    case ShipType::OTYPE_FACTORY:
    case ShipType::OTYPE_BERSCTLC:
    case ShipType::OTYPE_AUTOFAC:
    case ShipType::OTYPE_PLANDEF:
    case ShipType::OTYPE_QUARRY:
    case ShipType::OTYPE_DOME:
    case ShipType::OTYPE_WPLANT:
    case ShipType::OTYPE_PORT:
    case ShipType::OTYPE_ABM:
    case ShipType::OTYPE_AFV:
    case ShipType::OTYPE_BUNKER:
    case ShipType::STYPE_LANDER:
      return std::monostate{};
  }
  std::unreachable();
}

/// Returns true if `special` holds the expected variant alternative for `type`.
export [[nodiscard]] constexpr bool
holds_expected_special_data(ShipType type,
                            const SpecialData& special) noexcept {
  return special.index() == default_special_data(type).index();
}

/// Automated navigation course parameters for a ship.
export struct NavigateData {
  bool on{false};          ///< Whether navigation course mode is active
  speed_t speed{0};        ///< Dialed navigation speed throttle (0..9)
  std::uint32_t turns{0};  ///< Movement turns remaining in maneuver
  bearing_t bearing{0};    ///< Course heading in degrees (0..359)
};

/// Defensive escort and auto-retaliation parameters for a ship.
export struct ProtectData {
  shipnum_t ship{0};   ///< Target ship number being protected
  bool on{false};      ///< Whether escort / protection mode is active
  bool planet{false};  ///< Whether assigned as a planetary defense interceptor
  bool retaliate{
      false};         ///< Whether ship automatically retaliates when attacked
  bool evade{false};  ///< Whether ship executes evasive maneuvers in combat
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
  resource_t build_cost{0};  ///< Construction cost in resources

  double base_mass{0.0};   ///< Empty hull baseline mass
  double tech{0.0};        ///< Construction technology level
  double complexity{0.0};  ///< Hull structural complexity rating

  resource_t destruct{0};  ///< Current carried destructive charges
  resource_t resource{0};  ///< Current carried resource cargo
  population_t popn{0};    ///< Current carried colonists / crew
  population_t troops{0};  ///< Current carried military troops
  crystal_t crystals{0};   ///< Current carried warp crystal charge

  mutable SpecialData special;  ///< Ship-type-specific payload / mode data

  NavigateData navigate;  ///< Standing navigational heading orders
  ProtectData protect;    ///< Escort, defense, and evasion orders

  bool mount{false};             ///< Crystal mount equipped
  HyperDriveData hyper_drive;    ///< Hyperspace jump drive systems
  weapon_power_t cew{0};         ///< Concentrated energy weapon power rating
  weapon_range_t cew_range{0};   ///< CEW beam operational range
  bool cloak{false};             ///< Cloaking device equipped
  bool laser{false};             ///< Combat laser weapon equipped
  bool focus{false};             ///< Laser focus mode enabled
  weapon_power_t fire_laser{0};  ///< Armed combat laser firing strength

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

  ShipType type{ShipType::STYPE_POD};  ///< Operational ship type classification
  speed_t speed{0};                    ///< Current impulse speed throttle

  bool active{false};  ///< Operational / crewed status
  bool alive{false};   ///< Ship hull intact / not destroyed
  bool mode{
      false};  ///< Warhead detonation mode (false: explosive, true: radiative)
  bool bombard{false};  ///< Planetary bombardment enabled
  bool mounted{false};  ///< Warp crystal currently mounted in jump drive
  DockState dock_state{
      DockState::Spaceborne};  ///< Physical mooring or landing status
  bool notified{false};        ///< Player notified of arrival / event
  bool examined{false};        ///< Ship surveyed / examined
  bool on{false};              ///< Factory / power generator online

  int merchant{0};  ///< Assigned merchant shipping route (0 = off,
                    ///< 1..MAX_ROUTES = route number)
  ActiveBattery guns{ActiveBattery::NONE};  ///< Active gun battery mode
  GunBattery primary_battery;               ///< Primary gun battery
  GunBattery secondary_battery;             ///< Secondary gun battery

  hangar_t hanger{0};      ///< Current docked fighters / payload count
  hangar_t max_hanger{0};  ///< Maximum hangar capacity
};
