// SPDX-License-Identifier: Apache-2.0

/// \file ship_subclasses.cppm
/// \brief Specialized Ship domain subclasses and downcast type traits.

export module gb.entities:ship_subclasses;

import std;

import :planet;
import :sector;
import :ship_base;
import :ship_types;
import :types;

// =========================================================================
// AutonomousShip and Derived Specialty Subclasses
// =========================================================================

export class AutonomousShip : public Ship {
public:
  AutonomousShip() {
    data_.type = ShipType::OTYPE_VN;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<MindData>(data_.special)) {
      data_.special =
          MindData{.progenitor = data_.owner, .generation = 1, .busy = true};
    }
    return std::get<MindData>(data_.special);
  }
  [[nodiscard]] bool is_busy() const noexcept {
    return mind().busy;
  }
  void set_busy(bool busy) noexcept {
    mind().busy = busy;
  }

  [[nodiscard]] player_t progenitor() const noexcept {
    return mind().progenitor;
  }
  [[nodiscard]] player_t target() const noexcept {
    return mind().target;
  }
  void set_target(player_t target) noexcept {
    mind().target = target;
  }
  [[nodiscard]] player_t who_killed() const noexcept {
    return mind().who_killed;
  }
  void set_who_killed(player_t killer) noexcept {
    mind().who_killed = killer;
  }
  [[nodiscard]] std::uint32_t generation() const noexcept {
    return mind().generation;
  }
  [[nodiscard]] bool is_tampered() const noexcept {
    return mind().tampered;
  }
  void set_tampered(bool tampered) noexcept {
    mind().tampered = tampered;
  }

  /// \brief Mines resources from a sector, transferring extracted yield to
  /// cargo and fuel.
  resource_t mine_sector(Sector& sector);

  /// \brief Moves an autonomous machine to an adjacent sector when current
  /// sector is depleted.
  Coordinates roam_to_adjacent_sector(const Planet& planet);

  /// \brief Generates a random binary name for a new Von Neumann machine.
  [[nodiscard]] static std::string generate_binary_name();
};

export class VonNeumannShip : public AutonomousShip {
public:
  using AutonomousShip::AutonomousShip;
};

export class BerserkerShip : public AutonomousShip {
public:
  BerserkerShip() {
    data_.type = ShipType::OTYPE_BERS;
    ensure_valid_special_data();
  }
  using AutonomousShip::AutonomousShip;
};

export class SpaceMirrorShip : public Ship {
public:
  SpaceMirrorShip() {
    data_.type = ShipType::STYPE_MIRROR;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<AimedAtData>(data_.special)) {
      data_.special = AimedAtData{};
    }
    return std::get<AimedAtData>(data_.special);
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

  /// Calculates the 0..7 compass aim direction heading toward the given target
  /// coordinates.
  [[nodiscard]] int
  aim_direction(UniverseCoordinates target_coords) const noexcept;
};

export class SporePodShip : public Ship {
public:
  SporePodShip() {
    data_.type = ShipType::STYPE_POD;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<PodData>(data_.special)) {
      data_.special = PodData{};
    }
    return std::get<PodData>(data_.special);
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
  CanisterShip() {
    data_.type = ShipType::OTYPE_CANIST;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<TimerData>(data_.special)) {
      data_.special = TimerData{};
    }
    return std::get<TimerData>(data_.special);
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
  MissileShip() {
    data_.type = ShipType::STYPE_MISSILE;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<ImpactData>(data_.special)) {
      data_.special = ImpactData{};
    }
    return std::get<ImpactData>(data_.special);
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
  MineShip() {
    data_.type = ShipType::STYPE_MINE;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<TriggerData>(data_.special)) {
      data_.special = TriggerData{};
    }
    return std::get<TriggerData>(data_.special);
  }
  [[nodiscard]] weapon_range_t trigger_radius() const noexcept {
    return trigger().radius;
  }
  void set_trigger_radius(weapon_range_t radius) noexcept {
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
  TerraformerShip() {
    data_.type = ShipType::OTYPE_TERRA;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<TerraformData>(data_.special)) {
      data_.special = TerraformData{};
    }
    return std::get<TerraformData>(data_.special);
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
  GroundPlowShip() {
    data_.type = ShipType::OTYPE_PLOW;
    ensure_valid_special_data();
  }
  using TerraformerShip::TerraformerShip;
};

export class TransporterShip : public Ship {
public:
  TransporterShip() {
    data_.type = ShipType::OTYPE_TRANSDEV;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<TransportData>(data_.special)) {
      data_.special = TransportData{};
    }
    return std::get<TransportData>(data_.special);
  }
  [[nodiscard]] shipnum_t target_ship() const noexcept {
    return transport().target;
  }
  void set_target_ship(shipnum_t target) noexcept {
    transport().target = target;
  }
};

export class ToxicWasteShip : public Ship {
public:
  ToxicWasteShip() {
    data_.type = ShipType::OTYPE_TOXWC;
    ensure_valid_special_data();
  }
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
    if (!std::holds_alternative<WasteData>(data_.special)) {
      data_.special = WasteData{};
    }
    return std::get<WasteData>(data_.special);
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
    data_.dock_state = DockState::Spaceborne;
    destshipno() = shipno;
    whatdest() = level;
    deststar() = snum;
    destpnum() = pnum;
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

export template <>
struct ShipTypeTraits<AutonomousShip> {
  using payload_type = MindData;
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type == ShipType::OTYPE_VN || type == ShipType::OTYPE_BERS;
  }
};

export template <>
struct ShipTypeTraits<VonNeumannShip> {
  using payload_type = MindData;
  static constexpr ShipType expected_type = ShipType::OTYPE_VN;
};

export template <>
struct ShipTypeTraits<BerserkerShip> {
  using payload_type = MindData;
  static constexpr ShipType expected_type = ShipType::OTYPE_BERS;
};

export template <>
struct ShipTypeTraits<SpaceMirrorShip> {
  using payload_type = AimedAtData;
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type >= ShipType::STYPE_MIRROR && type <= ShipType::OTYPE_TRACT;
  }
};

export template <>
struct ShipTypeTraits<SporePodShip> {
  using payload_type = PodData;
  static constexpr ShipType expected_type = ShipType::STYPE_POD;
};

export template <>
struct ShipTypeTraits<CanisterShip> {
  using payload_type = TimerData;
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type == ShipType::OTYPE_CANIST || type == ShipType::OTYPE_GREEN;
  }
};

export template <>
struct ShipTypeTraits<MissileShip> {
  using payload_type = ImpactData;
  static constexpr ShipType expected_type = ShipType::STYPE_MISSILE;
};

export template <>
struct ShipTypeTraits<MineShip> {
  using payload_type = TriggerData;
  static constexpr ShipType expected_type = ShipType::STYPE_MINE;
};

export template <>
struct ShipTypeTraits<TerraformerShip> {
  using payload_type = TerraformData;
  [[nodiscard]] static constexpr bool matches(ShipType type) noexcept {
    return type == ShipType::OTYPE_TERRA || type == ShipType::OTYPE_PLOW;
  }
};

export template <>
struct ShipTypeTraits<GroundPlowShip> {
  using payload_type = TerraformData;
  static constexpr ShipType expected_type = ShipType::OTYPE_PLOW;
};

export template <>
struct ShipTypeTraits<TransporterShip> {
  using payload_type = TransportData;
  static constexpr ShipType expected_type = ShipType::OTYPE_TRANSDEV;
};

export template <>
struct ShipTypeTraits<ToxicWasteShip> {
  using payload_type = WasteData;
  static constexpr ShipType expected_type = ShipType::OTYPE_TOXWC;
};

// ShipFactory - instantiates polymorphic Ship subclasses from ship_struct or
// ShipType templates
export class ShipFactory {
public:
  [[nodiscard]] static std::unique_ptr<Ship> create(ship_struct data);
  [[nodiscard]] static std::unique_ptr<Ship>
  create_from_template(ShipType type, player_t owner = 1);
};

export enum class GroundMovementError {
  NotTerraformVehicle,
  Stopped,
  EmptyOrders,
  InvalidIndex,
};

export std::expected<char, GroundMovementError>
get_ground_order(const Ship& ship, std::size_t index);

/// \brief Result of a single ground vehicle movement step.
export struct GroundStepResult {
  Coordinates destination;
  bool bounced{false};

  constexpr bool operator==(const GroundStepResult&) const = default;
};

/// \brief Reflects a keypad movement direction vertically across a polar
/// boundary.
export constexpr char reflect_polar_order(char order) noexcept {
  switch (order) {
    case '1':
      return '7';
    case '2':
      return '8';
    case '3':
      return '9';
    case '7':
      return '1';
    case '8':
      return '2';
    case '9':
      return '3';
    default:
      return order;
  }
}

/// \brief Calculates destination coordinates and polar bounce status for a
/// ground movement step.
export GroundStepResult calculate_ground_step(const Planet& planet, char order,
                                              Coordinates from) noexcept;
