// SPDX-License-Identifier: Apache-2.0

/// \file ship_base.cc
/// \brief Base Ship domain methods, mass/cost/complexity calculations, and
/// cargo transfers.

module;

import std;

module gb.entities;

double Ship::base_mass() const noexcept {
  return 1.0 + MASS_ARMOR * armor() + MASS_SIZE * shipbody() +
         MASS_HANGER * max_hanger() +
         MASS_GUNS * primary_battery().mass_contribution() +
         MASS_GUNS * secondary_battery().mass_contribution();
}

double Ship::local_mass(double race_mass) const noexcept {
  return base_mass() + fuel() * MASS_FUEL +
         static_cast<double>(resource()) * MASS_RESOURCE +
         static_cast<double>(destruct()) * MASS_DESTRUCT +
         static_cast<double>(popn() + troops()) * race_mass;
}

ship_size_t Ship::calculate_size() const noexcept {
  const double calculated =
      1.0 + SIZE_GUNS * static_cast<double>(primary_battery().count) +
      SIZE_GUNS * static_cast<double>(secondary_battery().count) +
      SIZE_CREW * max_crew() + SIZE_RESOURCE * max_resource() +
      SIZE_FUEL * max_fuel() + SIZE_DESTRUCT * max_destruct() + max_hanger();
  return static_cast<ship_size_t>(std::floor(calculated));
}

resource_t cost(const Ship& s) {
  /* compute how much it costs to build this ship */
  double factor = 0.0;
  factor += static_cast<double>(ship_template(s.build_type()).build_cost);
  factor += GUN_COST * static_cast<double>(s.primary_battery().count);
  factor += GUN_COST * static_cast<double>(s.secondary_battery().count);
  factor += CREW_COST * static_cast<double>(s.max_crew());
  factor += CARGO_COST * static_cast<double>(s.max_resource());
  factor += FUEL_COST * static_cast<double>(s.max_fuel());
  factor += AMMO_COST * static_cast<double>(s.max_destruct());
  factor += SPEED_COST * static_cast<double>(s.max_speed()) *
            std::sqrt(static_cast<double>(s.max_speed()));
  factor += HANGER_COST * static_cast<double>(s.max_hanger());
  factor += ARMOR_COST * static_cast<double>(s.armor()) *
            std::sqrt(static_cast<double>(s.armor()));
  factor += CEW_COST * static_cast<double>(s.cew() * s.cew_range());
  /* additional advantages/disadvantages */

  double advantage = 0.0;
  advantage += 0.5 * !!s.hyper_drive().has;
  advantage += 0.5 * !!s.laser();
  advantage += 0.5 * !!s.cloak();
  advantage += 0.5 * !!s.mount();

  factor *= std::sqrt(1.0 + advantage);
  return static_cast<resource_t>(factor);
}

namespace {

/**
 * Accumulates advantage and disadvantage scores for ship customization.
 *
 * For each ship stat, compares the actual value against the baseline template.
 * Stats above baseline contribute to advantage; stats below contribute to
 * disadvantage.
 */
class SystemCost {
public:
  /**
   * Add a stat comparison to the running totals.
   *
   * @param value The ship's actual stat value.
   * @param base The baseline value from ShipTemplate.
   */
  void add(int value, int base) {
    const double ratio = ((static_cast<double>(value) + 1.0) /
                          (static_cast<double>(base) + 1.0)) -
                         1.0;
    if (ratio >= 0.0) {
      advantage_ += ratio;
    } else {
      disadvantage_ -= ratio;
    }
  }

  [[nodiscard]] std::pair<double, double> get() const {
    return {advantage_, disadvantage_};
  }

private:
  double advantage_ = 0.0;
  double disadvantage_ = 0.0;
};

void apply_blueprint_capabilities(ship_struct& data, const ShipTemplate& itmpl,
                                  const Race* race) noexcept {
  data.mount = itmpl.can_mount && (!race || race->discoveries.crystal);
  data.hyper_drive.has =
      itmpl.can_hyperjump && (!race || race->discoveries.hyperdrive);
  data.cloak = itmpl.can_cloak && (!race || race->discoveries.cloak);
  data.laser = itmpl.can_mount_laser && (!race || race->discoveries.laser);
}

void initialize_constructed_specialty(Ship& s, player_t owner) {
  switch (s.type()) {
    case ShipType::OTYPE_VN:
    case ShipType::OTYPE_BERS:
      if (auto* autonomous = s.as<AutonomousShip>()) {
        autonomous->mind() = MindData{.progenitor = owner,
                                      .target = 0,
                                      .generation = 1,
                                      .busy = true,
                                      .tampered = false,
                                      .who_killed = 0};
      }
      break;
    case ShipType::STYPE_MIRROR:
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE:
    case ShipType::OTYPE_TRACT:
      if (auto* mirror = s.as<SpaceMirrorShip>()) {
        mirror->aim() = AimedAtData{};
      }
      break;
    case ShipType::STYPE_POD:
      if (auto* pod = s.as<SporePodShip>()) {
        pod->pod() = PodData{};
      }
      break;
    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      if (auto* canister = s.as<CanisterShip>()) {
        canister->timer() = TimerData{};
      }
      break;
    case ShipType::STYPE_MISSILE:
      if (auto* missile = s.as<MissileShip>()) {
        missile->impact() = ImpactData{};
      }
      break;
    case ShipType::STYPE_MINE:
      if (auto* mine = s.as<MineShip>()) {
        mine->set_trigger_radius(100);
      }
      break;
    case ShipType::OTYPE_TERRA:
    case ShipType::OTYPE_PLOW:
      if (auto* terra = s.as<TerraformerShip>()) {
        terra->terraform() = TerraformData{};
      }
      break;
    case ShipType::OTYPE_TRANSDEV:
      if (auto* trans = s.as<TransporterShip>()) {
        trans->set_target_ship(shipnum_t{0});
      }
      break;
    case ShipType::OTYPE_TOXWC:
      if (auto* waste = s.as<ToxicWasteShip>()) {
        waste->waste() = WasteData{};
      }
      break;
    default:
      break;
  }
}

template <typename StockT, typename CapT>
[[nodiscard]] constexpr std::int64_t
clamp_cargo_transfer(std::int64_t requested, StockT available_stock,
                     StockT current_dest, CapT max_dest,
                     bool unlimited_dest = false) noexcept {
  const auto avail = static_cast<std::int64_t>(available_stock);
  if (requested <= 0 || avail <= 0) return 0;
  const auto cur = static_cast<std::int64_t>(current_dest);
  const auto max_cap = static_cast<std::int64_t>(max_dest);
  const std::int64_t capacity =
      unlimited_dest ? avail : std::max<std::int64_t>(0, max_cap - cur);
  return std::max<std::int64_t>(0, std::min({requested, avail, capacity}));
}

[[nodiscard]] std::int64_t
compute_cargo_transfer_amount(const Ship& src, const Ship& dst,
                              ShipCargoType cargo,
                              std::int64_t amount) noexcept {
  switch (cargo) {
    case ShipCargoType::Resource:
      return clamp_cargo_transfer(amount, src.resource(), dst.resource(),
                                  dst.max_resource_capacity(),
                                  dst.can_strap_cargo_to_hull());
    case ShipCargoType::Destruct:
      return clamp_cargo_transfer(amount, src.destruct(), dst.destruct(),
                                  dst.max_destruct_capacity());
    case ShipCargoType::Fuel:
      return clamp_cargo_transfer(amount, src.fuel(), dst.fuel(),
                                  dst.max_fuel_capacity());
    case ShipCargoType::Crystal:
      return clamp_cargo_transfer(amount, src.crystals(), dst.crystals(),
                                  dst.max_crystals_capacity());
    case ShipCargoType::Crew:
      return clamp_cargo_transfer(amount, src.popn(), dst.popn(),
                                  dst.max_crew_capacity());
    case ShipCargoType::Troops:
      return clamp_cargo_transfer(amount, src.troops(), dst.troops(),
                                  dst.available_mil());
  }
  return 0;
}

}  // namespace

/**
 * Calculates the complexity of a ship design.
 *
 * Complexity determines whether a race can build a customized ship design.
 * If complexity(ship) > race.tech, the ship cannot be built. It's also used
 * for sorting ships in display order.
 *
 * The algorithm compares the ship's stats against the base ShipTemplate:
 * - Stats above baseline accumulate as "advantage" (linear growth)
 * - Stats below baseline accumulate as "disadvantage" (exponential decay
 * penalty)
 *
 * These are combined into a deviation score, normalized by the ship's base tech
 * requirement (higher tech ships tolerate more customization), then squared to
 * make large deviations exponentially more expensive.
 *
 * A ship with no modifications returns exactly its base tech requirement.
 * Upgrades increase complexity; downgrades slightly decrease it.
 *
 * @param s The Ship object for which the complexity is calculated.
 * @return The complexity value of the ship.
 */
double complexity(const Ship& s) {
  const auto& tmpl = ship_template(s.build_type());
  SystemCost cost;

  cost.add(s.primary_battery().count, tmpl.max_guns);
  cost.add(s.secondary_battery().count, tmpl.max_guns);
  cost.add(s.max_crew(), tmpl.max_crew);
  cost.add(s.max_resource(), tmpl.max_resource);
  cost.add(s.max_fuel(), tmpl.max_fuel);
  cost.add(s.max_destruct(), tmpl.max_destruct);
  cost.add(s.max_speed(), tmpl.base_speed);
  cost.add(s.max_hanger(), tmpl.max_hangar);
  cost.add(s.armor(), tmpl.base_armor);

  const double base_tech = tmpl.base_tech;
  const auto [advantage, disadvantage] = cost.get();

  // Combine advantage and disadvantage into a single deviation score.
  // Result is 1.0 for unmodified ships, >1.0 for upgrades, <1.0 for downgrades.
  const double combined_deviation =
      std::sqrt((1.0 + advantage) * std::exp(-disadvantage / 10.0));

  // Normalize by base tech (higher tech ships tolerate more customization).
  const double normalized_deviation =
      (COMPLEXITY_FACTOR * (combined_deviation - 1.0) /
       std::sqrt(base_tech + 1.0)) +
      1.0;

  // Square to make large deviations exponentially more expensive.
  const double complexity_multiplier =
      normalized_deviation * normalized_deviation;

  return complexity_multiplier * base_tech;
}

void Ship::set_factory_blueprint(ShipType build_type,
                                 const Race* race) noexcept {
  const auto& itmpl = ship_template(build_type);
  data_.build_type = build_type;
  data_.armor = itmpl.base_armor;
  data_.guns = ActiveBattery::NONE;
  data_.primary_battery =
      GunBattery::create(itmpl.max_guns, shipdata_primary(build_type));
  data_.secondary_battery =
      GunBattery::create(itmpl.max_guns, shipdata_secondary(build_type));
  data_.max_crew = itmpl.max_crew;
  data_.max_resource = itmpl.max_resource;
  data_.max_hanger = itmpl.max_hangar;
  data_.max_fuel = itmpl.max_fuel;
  data_.max_destruct = itmpl.max_destruct;
  data_.max_speed = itmpl.base_speed;
  apply_blueprint_capabilities(data_, itmpl, race);
  data_.cew = 0;
  data_.mode = 0;
  data_.size = calculate_size();
  const bool free_build = race ? race->God : false;
  data_.build_cost = free_build ? 0 : ::cost(*this);
  data_.base_mass = base_mass();
  data_.complexity = ::complexity(*this);
}

void Ship::initialize_constructed_state(const Race& race, governor_t gov,
                                        double load_fuel,
                                        population_t load_crew) {
  data_.speed = max_speed_capacity();
  data_.owner = race.Playernum;
  data_.governor = gov;
  admin_override_fuel(race.God ? max_fuel_capacity() : load_fuel, race.mass);
  data_.popn = race.God ? max_crew_capacity() : load_crew;
  if (race.God) {
    data_.resource = max_resource_capacity();
    data_.destruct = max_destruct_capacity();
    data_.mounted = data_.mount;
  }
  data_.alive = true;
  data_.active = true;
  data_.protect.self = active_guns() > 0;
  admin_override_damage(race.God ? 0 : get_template().base_damage);
  data_.retaliate = data_.primary_battery.count;
  set_mass(local_mass(race.mass));
  initialize_constructed_specialty(*this, race.Playernum);
}

/// Determine whether the ship crashed or not.
std::tuple<bool, int>
Ship::roll_landing_crash(const double required_fuel) const noexcept {
  // Crash from insufficient fuel.
  if (fuel() < required_fuel) return {true, 0};

  // Damaged ships stand of chance of crash landing.
  if (auto roll = int_rand(1, 100); roll <= damage()) return {true, roll};

  // No crash.
  return {false, 0};
}

std::int64_t Ship::transfer_cargo_to(Ship& destination, ShipCargoType cargo,
                                     std::int64_t amount,
                                     double race_mass) noexcept {
  const auto to_transfer =
      compute_cargo_transfer_amount(*this, destination, cargo, amount);
  if (to_transfer <= 0) return 0;

  switch (cargo) {
    case ShipCargoType::Resource:
      consume_resource(to_transfer);
      destination.add_resource(to_transfer);
      break;
    case ShipCargoType::Destruct:
      consume_destruct(to_transfer);
      destination.add_destruct(to_transfer);
      break;
    case ShipCargoType::Fuel:
      consume_fuel(static_cast<double>(to_transfer));
      destination.add_fuel(static_cast<double>(to_transfer));
      break;
    case ShipCargoType::Crystal:
      consume_crystals(static_cast<crystal_t>(to_transfer));
      destination.add_crystals(static_cast<crystal_t>(to_transfer));
      break;
    case ShipCargoType::Crew:
      remove_popn(to_transfer, race_mass);
      destination.add_popn(to_transfer, race_mass);
      break;
    case ShipCargoType::Troops:
      remove_troops(to_transfer, race_mass);
      destination.add_troops(to_transfer, race_mass);
      break;
  }
  return to_transfer;
}

/*
 * range of telescopes, ground or space, given race and ship
 */
double tele_range(ShipType type, double tech) {
  if (type == ShipType::OTYPE_GTELE)
    return std::log1p((double)tech) * 400 + SYSTEMSIZE / 8;

  return std::log1p((double)tech) * 1500 + SYSTEMSIZE / 3;
}

double Ship::tele_range() const noexcept {
  return ::tele_range(type(), tech());
}

double Ship::refuel_from_gas_giant(const Planet& planet) {
  if (is_landed() || planet.type() != PlanetType::GASGIANT) {
    return 0.0;
  }

  double fadd = 0.0;
  switch (type()) {
    case ShipType::STYPE_TANKER:
      fadd = FUEL_GAS_ADD_TANKER;
      break;
    case ShipType::STYPE_HABITAT:
      fadd = FUEL_GAS_ADD_HABITAT;
      break;
    default:
      fadd = FUEL_GAS_ADD;
      break;
  }
  const double capacity = static_cast<double>(max_fuel_capacity()) - fuel();
  const double added = std::clamp(fadd, 0.0, std::max(0.0, capacity));
  if (added > 0.0) {
    add_fuel(added);
  }
  return added;
}

bool Ship::process_radiation(bool update) {
  if (!rad()) {
    return true;
  }
  bool is_mobile = true;
  /* irradiated ships are immobile if radiation check fails */
  if (success(rad())) {
    is_mobile = false;
  }
  if (update) {
    auto new_popn = round_rand(static_cast<double>(popn()) * 0.80);
    auto new_troops = round_rand(static_cast<double>(troops()) * 0.80);
    apply_casualties(popn() - new_popn, troops() - new_troops);
    auto repair_amt = (rad() >= REPAIR_RATE)
                          ? int_rand(0, static_cast<int>(REPAIR_RATE))
                          : int_rand(0, static_cast<int>(rad()));
    repair_radiation(static_cast<radiation_t>(repair_amt));
  }
  return is_mobile;
}

bool Ship::prepare_for_flight(bool update) {
  /* ship is active */
  active() = true;

  if (owner() == 0) {
    alive() = false;
  }

  if (!alive()) {
    return false;
  }

  /* repair radiation & check mobility */
  active() = process_radiation(update);

  if (!popn() && max_crew_capacity() && !docked()) {
    whatdest() = ScopeLevel::LEVEL_UNIV;
  }

  return true;
}

void Ship::sync_factory_technology(const Race& race) noexcept {
  if (type() == ShipType::OTYPE_FACTORY && !on()) {
    tech() = race.tech;
  }
}
