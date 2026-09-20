// SPDX-License-Identifier: Apache-2.0

/// \file ship_base.cppm
/// \brief Base Ship domain entity class and core ship calculations.

export module gb.entities:ship_base;

import std;

import :planet;
import :race;
import :sector;
import :ship_templates;
import :ship_types;
import :types;
import :tweakables;

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
  [[nodiscard]] constexpr armor_t armor() const noexcept {
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

  /// \brief Returns whether this factory ship has been programmed with a ship
  /// design.
  [[nodiscard]] bool has_factory_design() const noexcept {
    return data_.type == ShipType::OTYPE_FACTORY &&
           data_.build_type != ShipType::OTYPE_FACTORY;
  }

  /// \brief Initializes this factory ship's blueprint attributes from a target
  /// ship class template, applying optional racial technology discoveries.
  void set_factory_blueprint(ShipType build_type,
                             const Race* race = nullptr) noexcept;

  [[nodiscard]] resource_t build_cost() const {
    return data_.build_cost;
  }
  resource_t& build_cost() {
    return data_.build_cost;
  }

  /// \brief Calculates empty hull baseline mass based on armor, size, hangar,
  /// and gun batteries.
  [[nodiscard]] double base_mass() const noexcept;

  /// \brief Computes the physical hull size (volume) of this ship from its
  /// gun batteries, crew, cargo, fuel, destruct, and hangar capacities.
  [[nodiscard]] ship_size_t calculate_size() const noexcept;

  [[nodiscard]] constexpr double tech() const noexcept {
    return data_.tech;
  }
  double& tech() {
    return data_.tech;
  }

  /// \brief Returns the maximum effective range of this ship's gun weaponry.
  [[nodiscard]] constexpr double gun_range() const noexcept {
    return ::gun_range(data_.tech);
  }

  /// \brief Returns the telescope observation range for this ship.
  [[nodiscard]] double tele_range() const noexcept;

  /// \brief Returns the effective weapon caliber for combat resolution (lasers
  /// and mines fire as LIGHT; missiles fire as HEAVY; otherwise uses the
  /// active gun battery's caliber).
  [[nodiscard]] constexpr guntype_t current_caliber() const noexcept {
    if (laser() && fire_laser()) return guntype_t::LIGHT;
    if (type() == ShipType::STYPE_MINE) return guntype_t::LIGHT;
    if (type() == ShipType::STYPE_MISSILE) return guntype_t::HEAVY;
    return active_gun_caliber();
  }

  /// \brief Evaluates whether the ship crashes during a planetary landing
  /// due to insufficient fuel or hull damage.
  [[nodiscard]] std::tuple<bool, int>
  roll_landing_crash(double required_fuel) const noexcept;

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

  [[nodiscard]] constexpr population_t popn() const noexcept {
    return data_.popn;
  }
  population_t& popn() {
    return data_.popn;
  }

  [[nodiscard]] constexpr population_t troops() const noexcept {
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

  [[nodiscard]] weapon_range_t cew_range() const {
    return data_.cew_range;
  }
  weapon_range_t& cew_range() {
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

  [[nodiscard]] weapon_power_t fire_laser() const {
    return data_.fire_laser;
  }
  weapon_power_t& fire_laser() {
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
  [[nodiscard]] constexpr damage_t damage() const noexcept {
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

  [[nodiscard]] DockState dock_state() const noexcept {
    return data_.dock_state;
  }
  [[nodiscard]] bool docked() const noexcept {
    return data_.dock_state != DockState::Spaceborne;
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
  [[nodiscard]] int merchant() const {
    return data_.merchant;
  }
  int& merchant() {
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
    return data_.dock_state == DockState::Docked;
  }

  /// Whether ship is currently landed on a planet surface.
  [[nodiscard]] bool is_landed() const noexcept {
    return data_.dock_state == DockState::Landed;
  }

  /// Whether ship is spaceborne (orbiting or flying in deep space).
  [[nodiscard]] bool is_spaceborne() const noexcept {
    return data_.dock_state == DockState::Spaceborne;
  }

  /// Returns the carrier ship ID if currently docked inside a carrier hangar.
  [[nodiscard]] std::optional<shipnum_t> carrier_id() const noexcept {
    if (is_docked() && data_.whatorbits == ScopeLevel::LEVEL_SHIP) {
      return data_.destshipno;
    }
    return std::nullopt;
  }

  /// Returns the other ship ID if currently moored to another spaceborne ship.
  [[nodiscard]] std::optional<shipnum_t> moored_ship_id() const noexcept {
    if (is_docked() && data_.whatorbits != ScopeLevel::LEVEL_SHIP &&
        data_.whatdest == ScopeLevel::LEVEL_SHIP) {
      return data_.destshipno;
    }
    return std::nullopt;
  }

  /// Lands the ship on a planet surface.
  void land_on_planet() noexcept {
    data_.dock_state = DockState::Landed;
    data_.whatorbits = ScopeLevel::LEVEL_PLAN;
    data_.whatdest = ScopeLevel::LEVEL_PLAN;
  }

  /// Docks the ship inside a carrier ship's hangar.
  void dock_into_carrier(shipnum_t carrier) noexcept {
    data_.dock_state = DockState::Docked;
    data_.destshipno = carrier;
    data_.whatorbits = ScopeLevel::LEVEL_SHIP;
    data_.whatdest = ScopeLevel::LEVEL_SHIP;
  }

  /// Moors the ship to another spaceborne ship (ship-to-ship docking).
  ///
  /// Note: Unlike carrier hangar docking (`dock_into_carrier`), the ship
  /// remains in its existing orbital reference frame (`whatorbits`).
  void dock_with_ship(shipnum_t other_ship) noexcept {
    data_.dock_state = DockState::Docked;
    data_.destshipno = other_ship;
    data_.whatdest = ScopeLevel::LEVEL_SHIP;
  }

  /// Moors the ship to another spaceborne ship (overload taking Ship
  /// reference).
  void dock_with_ship(const Ship& other) noexcept {
    dock_with_ship(other.number());
  }

  /// Symmetrically moors this ship and other together in space.
  void moor_together(Ship& other) noexcept {
    dock_with_ship(other);
    other.dock_with_ship(*this);
  }

  /// Returns true if governor is authorized to command this ship (deity/leader
  /// governor 0 or the assigned governor).
  [[nodiscard]] bool is_authorized_for(governor_t gov) const noexcept {
    return gov == 0 || data_.governor == gov;
  }

  /// Returns true if this ship is alive, active, owned by player, and
  /// authorized for governor.
  [[nodiscard]] bool is_commandable_by(player_t player,
                                       governor_t gov) const noexcept {
    return alive() && active() && owner() == player && is_authorized_for(gov);
  }

  /// Initializes pure domain state for a newly constructed ship from template
  /// defaults, race attributes, and autoloaded crew/fuel quantities.
  void initialize_constructed_state(const Race& race, governor_t gov,
                                    double load_fuel, population_t load_crew);

  /// Computes the fuel required to maneuver and dock with or assault target.
  [[nodiscard]] double
  docking_fuel_cost(const Ship& target,
                    bool is_assault = false) const noexcept {
    const double dist = target.coordinates().distance_to(coordinates());
    const double multiplier = is_assault ? ASSAULT_FUEL_MULTIPLIER : 1.0;
    return DOCK_BASE_FUEL_COST + dist * DOCK_DISTANCE_FUEL_FACTOR * multiplier *
                                     std::sqrt(static_cast<double>(mass()));
  }

  /// Undocks the ship from a moored ship in space. Preserves whatorbits.
  void undock_from_ship() noexcept {
    data_.dock_state = DockState::Spaceborne;
    data_.destshipno = 0;
    data_.whatdest = ScopeLevel::LEVEL_UNIV;
  }

  /// Launches the ship into orbit or deep space.
  void
  launch_to_orbit(ScopeLevel orbit_level = ScopeLevel::LEVEL_PLAN) noexcept {
    data_.dock_state = DockState::Spaceborne;
    if (data_.whatorbits == ScopeLevel::LEVEL_SHIP) {
      data_.destshipno = 0;
    }
    if (orbit_level != ScopeLevel::LEVEL_SHIP) {
      data_.whatorbits = orbit_level;
    }
  }

  /// \brief Returns true if a craft of the given physical size can fit inside
  /// this ship's remaining hangar space.
  [[nodiscard]] bool can_fit_in_hangar(ship_size_t craft_size) const noexcept {
    return data_.hanger + craft_size <= data_.max_hanger;
  }

  /// \brief Returns true if the given child craft can fit inside this ship's
  /// remaining hangar space.
  [[nodiscard]] bool can_fit_in_hangar(const Ship& craft) const noexcept {
    return can_fit_in_hangar(craft.size());
  }

  /// \brief Embeds a child craft into this carrier's hangar, updating occupied
  /// hangar space and aggregate mass.
  void load_docked_craft(ship_size_t craft_size, double craft_mass) noexcept {
    data_.hanger += craft_size;
    data_.mass += craft_mass;
  }

  /// \brief Embeds a child craft into this carrier's hangar, updating occupied
  /// hangar space and aggregate mass.
  void load_docked_craft(const Ship& craft) noexcept {
    load_docked_craft(craft.size(), craft.mass());
  }

  /// \brief Removes a child craft from this carrier's hangar, updating occupied
  /// hangar space (clamped to 0) and aggregate mass.
  void unload_docked_craft(ship_size_t craft_size, double craft_mass) noexcept {
    data_.hanger = (data_.hanger > craft_size) ? data_.hanger - craft_size : 0;
    data_.mass -= craft_mass;
    if (data_.mass < base_mass()) {
      data_.mass = base_mass();
    }
  }

  /// \brief Removes a child craft from this carrier's hangar, updating occupied
  /// hangar space (clamped to 0) and aggregate mass.
  void unload_docked_craft(const Ship& craft) noexcept {
    unload_docked_craft(craft.size(), craft.mass());
  }

  /// \brief Atomically transfers cargo or personnel from this ship to a
  /// destination ship, updating inventories, capacity limits, and physical
  /// masses.
  /// \param destination Target vessel receiving the cargo.
  /// \param cargo Type of cargo or personnel being transferred.
  /// \param amount Maximum quantity to transfer (must be positive).
  /// \param race_mass Biological mass factor for crew and troops.
  /// \return Quantity of cargo actually transferred.
  std::int64_t transfer_cargo_to(Ship& destination, ShipCargoType cargo,
                                 std::int64_t amount,
                                 double race_mass = 1.0) noexcept;

  /// Whether ship has an active combat laser armed and ready to fire.
  [[nodiscard]] bool is_laser_on() const noexcept {
    return data_.laser && data_.fire_laser > 0;
  }

  /// \brief Computes kinetic retaliation firepower bounded by crew staffing,
  /// programmed salvo limit, and stored destruct munitions.
  [[nodiscard]] weapon_power_t retal_strength() const noexcept {
    if (!alive()) return 0;
    if (!get_template().base_speed && !is_landed()) return 0;
    if (!popn() && type() != ShipType::OTYPE_BERS) return 0;

    const auto* battery = active_gun_battery();
    if (!battery) return 0;

    weapon_power_t avail =
        (type() == ShipType::STYPE_FIGHTER || type() == ShipType::OTYPE_AFV ||
         type() == ShipType::OTYPE_BERS)
            ? battery->count
            : std::min(static_cast<weapon_power_t>(popn()), battery->count);

    avail = std::min(retaliate(), avail);
    return std::min(destruct_power(), avail);
  }

  /// \brief Computes effective defensive or offensive weapon strength (armed
  /// combat laser strength bounded by fuel, or kinetic retaliation strength).
  [[nodiscard]] weapon_power_t check_retal_strength() const noexcept {
    if (!active() || !alive()) return 0;
    if (is_laser_on()) {
      return std::min(fire_laser(),
                      static_cast<weapon_power_t>(
                          fuel() / ENERGY_WEAPON_FUEL_PER_STRENGTH));
    }
    return retal_strength();
  }

  /// \brief Computes effective orbital bombardment firepower based on gun
  /// capacity, hull efficiency, and available destruct munitions.
  [[nodiscard]] weapon_power_t bombardment_strength() const noexcept {
    const auto effective_guns = static_cast<weapon_power_t>(
        static_cast<double>(max_guns_capacity()) * hull_efficiency());
    return std::min(effective_guns, destruct_power());
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
  [[nodiscard]] constexpr armor_t effective_armor() const noexcept {
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

  /// \brief Available cargo capacity remaining for destructive munitions.
  [[nodiscard]] resource_t available_destruct_capacity() const noexcept {
    return std::max<resource_t>(0, max_destruct_capacity() - data_.destruct);
  }

  /// \brief Available cargo capacity remaining for warp crystals.
  [[nodiscard]] crystal_t available_crystals_capacity() const noexcept {
    const auto cap = max_crystals_capacity();
    return cap > data_.crystals ? cap - data_.crystals : 0;
  }

  /// \brief Returns carried destructive munitions expressed as weapon power
  /// (for warhead detonation yield and ammunition-limited salvo caps).
  [[nodiscard]] weapon_power_t destruct_power() const noexcept {
    return static_cast<weapon_power_t>(std::max<resource_t>(0, data_.destruct));
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

  /// Whether this vessel can strap mineral resources to its external hull
  /// beyond its internal cargo bay capacity (true for shuttles not berthed in a
  /// hangar).
  [[nodiscard]] bool can_strap_cargo_to_hull() const noexcept {
    return data_.type == ShipType::STYPE_SHUTTLE &&
           data_.whatorbits != ScopeLevel::LEVEL_SHIP;
  }

  /// Returns the current fuel level truncated to integer cargo units.
  [[nodiscard]] resource_t fuel_units() const noexcept {
    return data_.fuel > 0.0 ? static_cast<resource_t>(data_.fuel) : 0;
  }

  /// Returns remaining fuel capacity in integer cargo units.
  [[nodiscard]] resource_t available_fuel_capacity() const noexcept {
    const double diff = max_fuel_capacity() - data_.fuel;
    return diff > 0.0 ? static_cast<resource_t>(diff) : 0;
  }

  /// \brief Adds resources and increments ship mass accordingly, clamped to
  /// max resource capacity unless the ship can strap cargo to its external
  /// hull. If amt is negative, delegates to consume_resource(-amt).
  void add_resource(resource_t amt) noexcept {
    if (amt < 0) {
      consume_resource(-amt);
      return;
    }
    const auto max_cap = max_resource_capacity();
    if (!can_strap_cargo_to_hull() && data_.resource >= max_cap) return;
    const auto actual = can_strap_cargo_to_hull()
                            ? amt
                            : std::min(amt, max_cap - data_.resource);
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

  /// \brief Refuels ship in orbit around a gas giant planet based on ship type
  /// capacity. Returns amount of fuel added (0.0 if not in orbit or not a gas
  /// giant).
  double refuel_from_gas_giant(const Planet& planet);

  /// \brief Processes radiation effects on ship crew and accumulated radiation
  /// decay.
  /// \param update Whether this is a full turn update pass (true) or segment
  /// (false).
  /// \return Whether the ship remains active/mobile after radiation checks.
  bool process_radiation(bool update);

  /// \brief Prepares a ship for turn flight, validating ownership, alive
  /// status, radiation mobility, and derelict uncrewed drifting.
  /// \param update Whether this is a full turn update pass (true) or segment
  /// (false).
  /// \return True if ship is alive and ready for turn processing, false if dead
  /// or unowned.
  bool prepare_for_flight(bool update);

  /// \brief Synchronizes offline factory technological capability with current
  /// empire technology.
  void sync_factory_technology(const Race& race) noexcept;
};

// Type traits for zero-cost static downcasting (specialized in ship_subclasses)
export template <typename T>
struct ShipTypeTraits {
  static_assert(std::is_base_of_v<Ship, T>, "T must derive from Ship");
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

export resource_t cost(const Ship&);
export double complexity(const Ship&);
export double tele_range(ShipType tech_level, double base_range);

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
