// SPDX-License-Identifier: Apache-2.0

/// \file ships.cc
/// \brief Ship domain methods and helper calculations.

module;

import std;
#undef stdout

module gblib;

armor_t getdefense(EntityManager& em, const Ship& ship) {
  if (ship.is_landed()) {
    const auto* smap = em.peek_sectormap(ship.storbits(), ship.pnumorbits());
    if (!smap) return 0;
    const auto& sect = smap->get(ship.land_coords());
    return 2 * sect.defense_bonus();
  }
  // No defense
  return 0;
}

namespace {

constexpr double TAN_22_5_DEG = std::numbers::sqrt2 - 1.0;
constexpr double TAN_67_5_DEG = std::numbers::sqrt2 + 1.0;

[[nodiscard]] int octant_for_positive_dy(double slope) noexcept {
  if (slope < -TAN_67_5_DEG || slope > TAN_67_5_DEG) return 4;
  if (slope > TAN_22_5_DEG) return 3;
  if (slope > 0.000) return 2;
  if (slope > -TAN_22_5_DEG) return 6;
  return 5;
}

[[nodiscard]] int octant_for_negative_dy(double slope) noexcept {
  if (slope < -TAN_67_5_DEG || slope > TAN_67_5_DEG) return 0;
  if (slope > TAN_22_5_DEG) return 7;
  if (slope > 0.000) return 6;
  if (slope > -TAN_22_5_DEG) return 2;
  return 1;
}

}  // namespace

/// \brief Computes the 8-octant compass heading (0..7) toward the given
/// target coordinates.
///
/// The 8 compass directions correspond to:
/// - 0: North (0 deg)
/// - 1: North-East (45 deg)
/// - 2: East (90 deg)
/// - 3: South-East (135 deg)
/// - 4: South (180 deg)
/// - 5: South-West (225 deg)
/// - 6: West (270 deg)
/// - 7: North-West (315 deg)
///
/// The slope boundaries are based on the tangent of half-octant (22.5 deg)
/// boundaries:
/// - tan(22.5 deg) = sqrt(2) - 1 ≈ 0.4142
/// - tan(67.5 deg) = sqrt(2) + 1 ≈ 2.4142
///
/// \param target_coords Absolute universe coordinates of the target.
/// \return Compass direction heading index (0..7).
int SpaceMirrorShip::aim_direction(
    UniverseCoordinates target_coords) const noexcept {
  const auto [xt, yt] = target_coords;
  const auto my_coords = coordinates();
  if (xt == my_coords.x) {
    return (yt > my_coords.y) ? 4 : 0;
  }
  if (yt == my_coords.y) {
    return (xt > my_coords.x) ? 2 : 6;
  }

  const double slope = (yt - my_coords.y) / (xt - my_coords.x);
  return (yt > my_coords.y) ? octant_for_positive_dy(slope)
                            : octant_for_negative_dy(slope);
}

void capture_stuff(const Ship& ship, GameObj& g) {
  for (auto ship_handle :
       ShipList::in_carrier(g.entity_manager, ship.number())) {
    Ship& s = *ship_handle;
    capture_stuff(s, g); /* recursive call */
    s.owner() =
        ship.owner(); /* make sure he gets all of the ships landed on it */
    s.governor() = ship.governor();
    g.out << std::format("{} CAPTURED!\n", s);
  }
}

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

double getmass(const Ship& s) {
  return s.base_mass();
}

ship_size_t Ship::calculate_size() const noexcept {
  const double calculated =
      1.0 + SIZE_GUNS * static_cast<double>(primary_battery().count) +
      SIZE_GUNS * static_cast<double>(secondary_battery().count) +
      SIZE_CREW * max_crew() + SIZE_RESOURCE * max_resource() +
      SIZE_FUEL * max_fuel() + SIZE_DESTRUCT * max_destruct() + max_hanger();
  return static_cast<ship_size_t>(std::floor(calculated));
}

unsigned int ship_size(const Ship& s) {
  return s.calculate_size();
}

resource_t cost(const Ship& s) {
  /* compute how much it costs to build this ship */
  double factor = 0.0;
  factor += static_cast<double>(ship_template(s.build_type()).build_cost);
  factor += GUN_COST * static_cast<double>(s.primary_battery().count);
  factor += GUN_COST * static_cast<double>(s.secondary_battery().count);
  factor += CREW_COST * (double)s.max_crew();
  factor += CARGO_COST * (double)s.max_resource();
  factor += FUEL_COST * (double)s.max_fuel();
  factor += AMMO_COST * (double)s.max_destruct();
  factor += SPEED_COST * (double)s.max_speed() *
            (double)std::sqrt((double)s.max_speed());
  factor += HANGER_COST * (double)s.max_hanger();
  factor +=
      ARMOR_COST * (double)s.armor() * (double)std::sqrt((double)s.armor());
  factor += CEW_COST * (double)(s.cew() * s.cew_range());
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

[[nodiscard]] bool merchant_land_ship(EntityManager& em, Ship& s,
                                      const Planet& p,
                                      const Coordinates& dest_coords,
                                      std::stringstream& telegram) {
  if (s.is_landed()) return true;

  const double fuel = s.mass() * p.gravity() * LAND_GRAV_MASS_FACTOR;
  if (s.fuel() < fuel) {
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
    telegram << "\t\tNot enough fuel to land!\n";
    return false;
  }
  s.set_land_coords(dest_coords);
  telegram << std::format("\t\tLanded on sector {}\n", s.land_coords());
  const auto& star = *em.peek_star(s.storbits());
  s.set_coordinates(p.absolute_coordinates(star));
  use_fuel(s, fuel);
  s.land_on_planet();
  s.deststar() = s.storbits();
  s.destpnum() = s.pnumorbits();
  return true;
}

void merchant_load_cargo(Ship& s, plinfo& pinfo, const auto& load,
                         std::stringstream& telegram) {
  if (!load.any()) return;

  telegram << "\t\t";
  if (load.fuel) {
    const int room = std::max(0, static_cast<int>(s.max_fuel_capacity()) -
                                     static_cast<int>(s.fuel()));
    const int amount = std::clamp<int>(room, 0, pinfo.fuel);
    pinfo.fuel -= amount;
    rcv_fuel(s, static_cast<double>(amount));
    telegram << std::format("{}f ", amount);
  }
  if (load.resources) {
    const int room = std::max(0, static_cast<int>(s.max_resource_capacity()) -
                                     static_cast<int>(s.resource()));
    const int amount = std::clamp<int>(room, 0, pinfo.resource);
    pinfo.resource -= amount;
    rcv_resource(s, amount);
    telegram << std::format("{}r ", amount);
  }
  if (load.crystals) {
    const int room = std::max(0, static_cast<int>(s.max_crystals_capacity()) -
                                     static_cast<int>(s.crystals()));
    const int amount = std::clamp<int>(room, 0, pinfo.crystals);
    pinfo.crystals -= amount;
    s.add_crystals(amount);
    telegram << std::format("{}x ", amount);
  }
  if (load.destruct) {
    const int room = std::max(0, static_cast<int>(s.max_destruct_capacity()) -
                                     static_cast<int>(s.destruct()));
    const int amount = std::clamp<int>(room, 0, pinfo.destruct);
    pinfo.destruct -= amount;
    rcv_destruct(s, amount);
    telegram << std::format("{}d ", amount);
  }
  telegram << "loaded\n";
}

void merchant_unload_cargo(Ship& s, plinfo& pinfo, const auto& unload,
                           std::stringstream& telegram) {
  if (!unload.any()) return;

  telegram << "\t\t";
  if (unload.fuel) {
    const int amount = static_cast<int>(s.fuel());
    pinfo.fuel += amount;
    telegram << std::format("{}f ", amount);
    use_fuel(s, static_cast<double>(amount));
  }
  if (unload.resources) {
    const int amount = s.resource();
    pinfo.resource += amount;
    telegram << std::format("{}r ", amount);
    use_resource(s, amount);
  }
  if (unload.crystals) {
    const int amount = s.crystals();
    pinfo.crystals += amount;
    telegram << std::format("{}x ", amount);
    s.consume_crystals(amount);
  }
  if (unload.destruct) {
    const int amount = s.destruct();
    pinfo.destruct += amount;
    telegram << std::format("{}d ", amount);
    use_destruct(s, amount);
  }
  telegram << "unloaded\n";
}

void merchant_launch_to_next_stop(EntityManager& em, Ship& s, const Planet& p,
                                  const plroute& route,
                                  std::stringstream& telegram) {
  const double fuel = s.mass() * p.gravity() * LAUNCH_GRAV_MASS_FACTOR;
  if (s.fuel() < fuel) {
    telegram << "\t\tNot enough fuel to launch!\n";
    return;
  }
  s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
  s.deststar() = route.dest_star;
  s.destpnum() = route.dest_planet;
  use_fuel(s, fuel);
  telegram << std::format("\t\tDestination set to {}\n",
                          format_ship_dest(em, s));
  if (s.hyper_drive().has && s.storbits() != s.deststar()) {
    s.navigate().on = false;
    s.hyper_drive().on = true;
    s.hyper_drive().charge = s.mounted() ? HYPER_DRIVE_READY_CHARGE : 0;
    telegram << "\t\tJump orders set\n";
  }
}

/* this routine will do landing, launching, loading, unloading, etc
        for merchant ships. The ship is within landing distance of
        the target Planet */
static bool do_merchant(EntityManager& em, Ship& s, Planet& p,
                        std::stringstream& telegram) {
  if (!s.merchant()) {
    return false;
  }
  const player_t owner = s.owner();
  const auto& route = p.info(owner).route_at(s.merchant());
  if (!route.set) {
    return false;
  }
  const auto* smap = em.peek_sectormap(s.storbits(), s.pnumorbits());
  if (!smap) return false;
  const auto& sect = smap->get(route.dest_coords);
  if (sect.get_owner() != 0 && sect.get_owner() != owner) {
    return false;
  }

  if (!merchant_land_ship(em, s, p, route.dest_coords, telegram)) {
    return true;
  }
  merchant_load_cargo(s, p.info(owner), route.load, telegram);
  merchant_unload_cargo(s, p.info(owner), route.unload, telegram);
  merchant_launch_to_next_stop(em, s, p, route, telegram);
  return true;
}

void apply_blueprint_capabilities(ship_struct& data, const ShipTemplate& itmpl,
                                  const Race* race) noexcept {
  data.mount = itmpl.can_mount && (!race || race->discoveries.crystal);
  data.hyper_drive.has =
      itmpl.can_hyperjump && (!race || race->discoveries.hyperdrive);
  data.cloak = itmpl.can_cloak && (!race || race->discoveries.cloak);
  data.laser = itmpl.can_mount_laser && (!race || race->discoveries.laser);
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

/**
 * @brief Calculate the complexity (tech level) for a default ship of a type.
 *
 * For a ship with no modifications, this returns exactly its base tech
 * requirement. This is useful for sorting ship types by their base complexity.
 *
 * @param type The ShipType to get default complexity for.
 * @return The base complexity value for this ship type.
 */
double complexity(ShipType type) {
  // For an unmodified ship, complexity() returns exactly the base tech.
  // We can compute this directly without creating a full Ship object.
  return ship_template(type).base_tech;
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

namespace {

void initialize_constructed_specialty(Ship& s, player_t owner) {
  switch (s.type()) {
    case ShipType::OTYPE_VN:
      if (auto* vn = s.as<VonNeumannShip>()) {
        vn->mind() = MindData{.progenitor = owner,
                              .target = 0,
                              .generation = 1,
                              .busy = 1,
                              .tampered = 0,
                              .who_killed = 0};
      }
      break;
    case ShipType::STYPE_MINE:
      if (auto* mine = s.as<MineShip>()) {
        mine->set_trigger_radius(100);
      }
      break;
    case ShipType::OTYPE_TRANSDEV:
      if (auto* trans = s.as<TransporterShip>()) {
        trans->set_target_ship(shipnum_t{0});
      }
      break;
    default:
      break;
  }
}

}  // namespace

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

std::string dispshiploc_brief(EntityManager& em, const Ship& ship) {
  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_STAR: {
      const auto& star = *em.peek_star(ship.storbits());
      return std::format("/{0:4.4s}", star.get_name());
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto& star = *em.peek_star(ship.storbits());
      return std::format("/{0}/{1:4.4s}", star.get_name(),
                         star.get_planet_name(ship.pnumorbits()));
    }
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{0}", ship.destshipno());
    case ScopeLevel::LEVEL_UNIV:
      return "/";
  }
}

std::string dispshiploc(EntityManager& em, const Ship& ship) {
  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = em.peek_star(ship.storbits());
      return std::format("/{0}", star->get_name());
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = em.peek_star(ship.storbits());
      return std::format("/{0}/{1}", star->get_name(),
                         star->get_planet_name(ship.pnumorbits()));
    }
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{0}", ship.destshipno());
    case ScopeLevel::LEVEL_UNIV:
      return "/";
  }
}

/// Determine whether the ship crashed or not.
std::tuple<bool, int> crash(const Ship& s, const double fuel) noexcept {
  // Crash from insufficient fuel.
  if (s.fuel() < fuel) return {true, 0};

  // Damaged ships stand of chance of crash landing.
  if (auto roll = int_rand(1, 100); roll <= s.damage()) return {true, roll};

  // No crash.
  return {false, 0};
}

std::string prin_ship_orbits(EntityManager& em, const Ship& s) {
  switch (s.whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      return std::format("/({:0.0},{:1.0})", s.coordinates().x,
                         s.coordinates().y);
    case ScopeLevel::LEVEL_STAR:
      if (const auto* star = em.peek_star(s.storbits())) {
        return std::format("/{0}", star->get_name());
      }
      return "/";
    case ScopeLevel::LEVEL_PLAN:
      if (const auto* star = em.peek_star(s.storbits())) {
        return std::format("/{0}/{1}", star->get_name(),
                           star->get_planet_name(s.pnumorbits()));
      }
      return "/";
    case ScopeLevel::LEVEL_SHIP:
      if (const auto* mothership = em.peek_ship(s.destshipno())) {
        return prin_ship_orbits(em, *mothership);
      } else {
        return "/";
      }
  }
}

std::string format_ship_dest(EntityManager& em, const Ship& ship) {
  switch (ship.whatdest()) {
    case ScopeLevel::LEVEL_UNIV:
      return "/";
    case ScopeLevel::LEVEL_STAR:
      if (const auto* star = em.peek_star(ship.deststar())) {
        return std::format("/{}", star->get_name());
      }
      return "/";
    case ScopeLevel::LEVEL_PLAN:
      if (const auto* star = em.peek_star(ship.deststar())) {
        return std::format("/{}/{}", star->get_name(),
                           star->get_planet_name(ship.destpnum()));
      }
      return "/";
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{}", ship.destshipno());
  }
}

namespace {

constexpr double DEGREES_TO_RADIANS = std::numbers::pi / 180.0;

[[nodiscard]] double compute_hyperjump_fuel(const Ship& s,
                                            double dist) noexcept {
  const double distfac = HYPER_DIST_FACTOR * (s.tech() + 100.0);
  const double ratio = dist / distfac;
  if (s.mounted() && dist > distfac) {
    return HYPER_DRIVE_FUEL_USE * std::sqrt(s.mass()) * ratio;
  }
  return HYPER_DRIVE_FUEL_USE * std::sqrt(s.mass()) * ratio * ratio;
}

void charge_hyperdrive_capacitor(Ship& s) noexcept {
  if (s.mounted()) {
    s.hyper_drive().charge = HYPER_DRIVE_READY_CHARGE;
  } else if (s.hyper_drive().charge < HYPER_DRIVE_READY_CHARGE) {
    s.hyper_drive().charge += 1;
  }
}

void execute_hyperdrive_jump(EntityManager& em, Ship& s, bool is_update,
                             bool send_messages) {
  if (!is_update) return; /* we're not ready to jump until the update */

  if (!s.hyper_drive().is_ready()) {
    charge_hyperdrive_capacitor(s);
    return;
  }

  const auto* dest_star = em.peek_star(s.deststar());
  if (!dest_star) return;

  const double dist = s.coordinates().distance_to(dest_star->coordinates());
  const double fuse = compute_hyperjump_fuel(s, dist);
  if (s.fuel() < fuse) {
    if (send_messages) {
      push_telegram(em, s.owner(), s.governor(),
                    std::format("{} at system {} does not have {:.1f}f to do "
                                "hyperspace jump.",
                                s, prin_ship_orbits(em, s), fuse));
    }
    s.hyper_drive().on = false;
    return;
  }

  use_fuel(s, fuse);
  const double heading =
      std::atan2(dest_star->coordinates().x - s.coordinates().x,
                 dest_star->coordinates().y - s.coordinates().y);
  const double sn = std::sin(heading);
  const double cs = std::cos(heading);
  s.set_coordinates(
      UniverseCoordinates{dest_star->coordinates().x - sn * 0.9 * SYSTEMSIZE,
                          dest_star->coordinates().y - cs * 0.9 * SYSTEMSIZE});
  s.whatorbits() = ScopeLevel::LEVEL_STAR;
  s.storbits() = s.deststar();
  s.protect().planet = false;
  s.hyper_drive().on = false;
  s.hyper_drive().charge = 0;
  if (send_messages) {
    push_telegram(em, s.owner(), s.governor(),
                  std::format("{} arrived at {}.", s, prin_ship_orbits(em, s)));
  }
}

[[nodiscard]] bool is_expendable_or_probe_ship(const Ship& s) noexcept {
  return s.build_cost() <= 50 || s.type() == ShipType::OTYPE_VN ||
         s.type() == ShipType::OTYPE_BERS;
}

void handle_sublight_out_of_fuel(EntityManager& em, Ship& s,
                                 bool send_messages) {
  if (!send_messages) return;

  msg_OOF(em, s);
  if (s.whatorbits() == ScopeLevel::LEVEL_UNIV &&
      is_expendable_or_probe_ship(s)) {
    push_telegram(em, s.owner(), s.governor(),
                  std::format("{} has been lost in deep space.", s));
    em.kill_ship(s.owner(), s);
  }
}

void check_and_break_orbit(EntityManager& em, Ship& s) {
  if (s.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    const auto* ost = em.peek_star(s.storbits());
    const auto* opl = em.peek_planet(s.storbits(), s.pnumorbits());
    if (ost && opl &&
        s.coordinates().distance_to(opl->absolute_coordinates(*ost)) >
            PLORBITSIZE) {
      s.whatorbits() = ScopeLevel::LEVEL_STAR;
      s.protect().planet = false;
    }
  } else if (s.whatorbits() == ScopeLevel::LEVEL_STAR) {
    const auto* ost = em.peek_star(s.storbits());
    if (ost && s.coordinates().distance_to(ost->coordinates()) > SYSTEMSIZE) {
      s.whatorbits() = ScopeLevel::LEVEL_UNIV;
      s.protect().evade = false;
      s.protect().planet = false;
    }
  }
}

[[nodiscard]] double
compute_sublight_move_factor(const Ship& s, segments_t segments) noexcept {
  return SHIP_MOVE_SCALE * (1.0 - 0.01 * static_cast<double>(s.rad())) *
         (1.0 - 0.01 * static_cast<double>(s.damage())) *
         SpeedConsts[s.speed()] * MoveConsts[s.whatorbits()] /
         static_cast<double>(segments);
}

void execute_navigation_step(EntityManager& em, Ship& s, double fuse,
                             double mfactor) {
  const double heading = DEGREES_TO_RADIANS * s.navigate().bearing;
  use_fuel(s, fuse);
  const double sn = std::sin(heading);
  const double cs = std::cos(heading);
  s.set_coordinates(s.coordinates() +
                    SystemCoordinates{sn * mfactor, -cs * mfactor});
  s.navigate().turns--;
  if (!s.navigate().turns) {
    s.navigate().on = false;
  }
  check_and_break_orbit(em, s);
}

struct ResolvedDestination {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t star{0};
  planetnum_t pnum{0};
  UniverseCoordinates coords{};
  const Ship* target_ship{nullptr};
  bool valid{true};
};

void resolve_ship_destination_target(EntityManager& em, Ship& s,
                                     ResolvedDestination& res) {
  res.target_ship = em.peek_ship(s.destshipno());
  if (!res.target_ship) {
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
    s.protect().evade = false;
    res.valid = false;
    return;
  }
  s.deststar() = res.target_ship->storbits();
  s.destpnum() = res.target_ship->pnumorbits();
  res.star = s.deststar();
  res.pnum = s.destpnum();
  res.coords = res.target_ship->coordinates();

  const ScopeLevel dsh_orbit = res.target_ship->whatorbits();
  if (dsh_orbit == ScopeLevel::LEVEL_PLAN &&
      (s.whatorbits() != ScopeLevel::LEVEL_PLAN ||
       s.pnumorbits() != res.target_ship->pnumorbits())) {
    res.level = ScopeLevel::LEVEL_PLAN;
  } else if (dsh_orbit == ScopeLevel::LEVEL_STAR &&
             (s.whatorbits() != ScopeLevel::LEVEL_STAR ||
              s.storbits() != res.target_ship->storbits())) {
    res.level = ScopeLevel::LEVEL_STAR;
  }
}

[[nodiscard]] bool needs_interstellar_approach(const Ship& s,
                                               ScopeLevel level) noexcept {
  if (level == ScopeLevel::LEVEL_STAR) return true;
  if (level != ScopeLevel::LEVEL_PLAN) return false;
  return s.storbits() != s.deststar() ||
         s.whatorbits() == ScopeLevel::LEVEL_UNIV;
}

[[nodiscard]] ResolvedDestination resolve_destination_target(EntityManager& em,
                                                             Ship& s) {
  ResolvedDestination res{
      .level = s.whatdest(),
      .star = s.deststar(),
      .pnum = s.destpnum(),
  };

  if (res.level == ScopeLevel::LEVEL_SHIP) {
    resolve_ship_destination_target(em, s, res);
    if (!res.valid) return res;
  }

  if (needs_interstellar_approach(s, res.level)) {
    res.level = ScopeLevel::LEVEL_STAR;
    res.star = s.deststar();
    res.coords = em.peek_star(res.star)->coordinates();
  } else if (res.level == ScopeLevel::LEVEL_PLAN &&
             s.storbits() == s.deststar()) {
    res.level = ScopeLevel::LEVEL_PLAN;
    res.star = s.deststar();
    res.pnum = s.destpnum();
    const auto& dest_star = *em.peek_star(res.star);
    const auto& dest_planet = *em.peek_planet(res.star, res.pnum);
    res.coords = dest_planet.absolute_coordinates(dest_star);
  }
  return res;
}

[[nodiscard]] bool can_ship_explore(const Ship& s,
                                    bool checking_fuel) noexcept {
  if (checking_fuel) return false;
  return s.popn() > 0 || s.type() == ShipType::OTYPE_PROBE;
}

void explore_arrived_star(EntityManager& em, const Ship& s,
                          starnum_t deststar) {
  em.mutate_star(deststar, [&](Star& dst_star) {
    if (!dst_star.is_inhabited_by(s.owner())) {
      dst_star.governor(s.owner()) = s.governor();
    }
    dst_star.mark_explored_by(s.owner());
    dst_star.mark_inhabited_by(s.owner());
  });
}

void handle_star_arrival(EntityManager& em, Ship& s, starnum_t deststar,
                         bool send_messages, bool checking_fuel) {
  const auto& dst = *em.peek_star(deststar);
  if (s.coordinates().distance_to(dst.coordinates()) > SYSTEMSIZE * 1.5) {
    return;
  }

  s.whatorbits() = ScopeLevel::LEVEL_STAR;
  s.protect().planet = false;
  s.storbits() = deststar;
  if (can_ship_explore(s, checking_fuel)) {
    explore_arrived_star(em, s, deststar);
  }
  if (send_messages && s.type() != ShipType::OTYPE_VN) {
    push_telegram(em, s.owner(), s.governor(),
                  std::format("{} arrived at {}.", s, prin_ship_orbits(em, s)));
  }
  if (s.whatdest() == ScopeLevel::LEVEL_STAR) {
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
  }
}

void explore_arrived_planet(EntityManager& em, const Ship& s,
                            starnum_t deststar, planetnum_t destpnum) {
  em.mutate_planet(deststar, destpnum,
                   [&](Planet& p) { p.info(s.owner()).explored = 1; });
  em.mutate_star(deststar, [&](Star& dst_star) {
    dst_star.mark_explored_by(s.owner());
    dst_star.mark_inhabited_by(s.owner());
  });
}

void handle_planet_landing_distance(EntityManager& em, Ship& s,
                                    starnum_t deststar, planetnum_t destpnum,
                                    bool checking_fuel,
                                    std::stringstream& telegram) {
  telegram << std::format("{} within landing distance of {}.", s,
                          prin_ship_orbits(em, s));
  em.mutate_planet(deststar, destpnum, [&](Planet& dpl_planet) {
    const bool merch =
        checking_fuel ? false : do_merchant(em, s, dpl_planet, telegram);
    if (!merch && s.whatdest() == ScopeLevel::LEVEL_PLAN) {
      s.whatdest() = ScopeLevel::LEVEL_UNIV;
    }
  });
}

void handle_planet_arrival(EntityManager& em, Ship& s, starnum_t deststar,
                           planetnum_t destpnum, bool send_messages,
                           bool checking_fuel) {
  const auto& dst = *em.peek_star(deststar);
  const auto& dpl = *em.peek_planet(deststar, destpnum);
  const double dist =
      s.coordinates().distance_to(dpl.absolute_coordinates(dst));
  if (dist > PLORBITSIZE) return;

  if (can_ship_explore(s, checking_fuel)) {
    explore_arrived_planet(em, s, deststar, destpnum);
  }
  s.whatorbits() = ScopeLevel::LEVEL_PLAN;
  s.pnumorbits() = destpnum;

  std::stringstream telegram;
  if (dist <= static_cast<double>(DIST_TO_LAND)) {
    handle_planet_landing_distance(em, s, deststar, destpnum, checking_fuel,
                                   telegram);
  } else {
    telegram << std::format("{} arriving at {}.", s, prin_ship_orbits(em, s));
  }
  if (s.type() == ShipType::STYPE_OAP) {
    telegram << "\nEnslavement of the planet is now possible.";
  }
  if (send_messages && s.type() != ShipType::OTYPE_VN) {
    push_telegram(em, s.owner(), s.governor(), telegram.str());
  }
}

void handle_ship_arrival(Ship& s, const Ship& dsh) {
  if (s.coordinates().distance_to(dsh.coordinates()) > PLORBITSIZE) return;

  if (dsh.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    s.whatorbits() = ScopeLevel::LEVEL_PLAN;
    s.storbits() = dsh.storbits();
    s.pnumorbits() = dsh.pnumorbits();
  } else if (dsh.whatorbits() == ScopeLevel::LEVEL_STAR) {
    s.whatorbits() = ScopeLevel::LEVEL_STAR;
    s.storbits() = dsh.storbits();
    s.protect().planet = false;
  }
}

[[nodiscard]] bool is_already_at_destination(const Ship& s,
                                             const ResolvedDestination& dest,
                                             double truedist) noexcept {
  if (truedist >= DIST_TO_LAND) return false;
  if (s.whatorbits() != dest.level) return false;
  return s.storbits() == dest.star && s.pnumorbits() == dest.pnum;
}

[[nodiscard]] double adjust_approach_distance(const Ship& s,
                                              const ResolvedDestination& dest,
                                              double truedist) noexcept {
  if (dest.level == ScopeLevel::LEVEL_STAR &&
      (s.storbits() != dest.star || s.whatorbits() == ScopeLevel::LEVEL_UNIV)) {
    return truedist - SYSTEMSIZE * 0.90;
  }
  if (dest.level == ScopeLevel::LEVEL_PLAN &&
      s.whatorbits() == ScopeLevel::LEVEL_STAR && s.storbits() == dest.star &&
      truedist >= PLORBITSIZE) {
    return truedist - PLORBITSIZE * 0.90;
  }
  return truedist;
}

void dispatch_destination_arrival(EntityManager& em, Ship& s,
                                  const ResolvedDestination& dest,
                                  bool send_messages, bool checking_fuel) {
  if (needs_interstellar_approach(s, dest.level)) {
    handle_star_arrival(em, s, dest.star, send_messages, checking_fuel);
  } else if (dest.level == ScopeLevel::LEVEL_PLAN &&
             dest.star == s.storbits()) {
    handle_planet_arrival(em, s, dest.star, dest.pnum, send_messages,
                          checking_fuel);
  } else if (dest.level == ScopeLevel::LEVEL_SHIP && dest.target_ship) {
    handle_ship_arrival(s, *dest.target_ship);
  }
}

void execute_destination_step(EntityManager& em, Ship& s, double fuse,
                              double mfactor, bool send_messages,
                              bool checking_fuel) {
  const auto dest = resolve_destination_target(em, s);
  if (!dest.valid) return;

  const double truedist = s.coordinates().distance_to(dest.coords);
  if (is_already_at_destination(s, dest, truedist)) {
    return;
  }

  if (s.whatdest() == ScopeLevel::LEVEL_SHIP &&
      (!dest.target_ship || !followable(em, s, *dest.target_ship))) {
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
    s.protect().evade = false;
    if (send_messages) {
      push_telegram(em, s.owner(), s.governor(),
                    std::format("{} at {} lost sight of destination ship #{}.",
                                s, prin_ship_orbits(em, s), s.destshipno()));
    }
    return;
  }

  if (truedist > DIST_TO_LAND) {
    use_fuel(s, fuse);
    const double movedist = adjust_approach_distance(s, dest, truedist);
    const double heading = std::atan2(dest.coords.x - s.coordinates().x,
                                      -dest.coords.y + s.coordinates().y);
    const double step = std::min(mfactor, movedist);
    s.set_coordinates(
        s.coordinates() +
        SystemCoordinates{std::sin(heading) * step, -std::cos(heading) * step});
  }

  check_and_break_orbit(em, s);
  dispatch_destination_arrival(em, s, dest, send_messages, checking_fuel);
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

[[nodiscard]] bool can_ship_move_sublight(const Ship& s) noexcept {
  if (!s.speed() || s.docked() || !s.alive()) return false;
  return s.whatdest() != ScopeLevel::LEVEL_UNIV || s.navigate().on;
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

void moveship(EntityManager& em, Ship& s, bool is_update, bool send_messages,
              bool checking_fuel) {
  const auto* state = em.peek_server_state();
  if (!state) return;  // Can't move ships without knowing segments

  if (s.hyper_drive().has && s.hyper_drive().on) {
    execute_hyperdrive_jump(em, s, is_update, send_messages);
    return;
  }

  if (!can_ship_move_sublight(s)) {
    return;
  }

  // Normal sublight cruise burns 0.5x the base fuel rate; evasive maneuvers
  // double fuel consumption to the full 1.0x rate.
  const double evade_multiplier = s.protect().evade ? 1.0 : 0.5;
  const double fuse = evade_multiplier * s.speed() * s.mass() * FUEL_USE /
                      static_cast<double>(state->segments);
  if (s.fuel() < fuse) {
    handle_sublight_out_of_fuel(em, s, send_messages);
    return;
  }

  const double mfactor = compute_sublight_move_factor(s, state->segments);
  if (s.navigate().on) {
    execute_navigation_step(em, s, fuse, mfactor);
  } else {
    execute_destination_step(em, s, fuse, mfactor, send_messages,
                             checking_fuel);
  }
}

/* deliver an "out of fuel" message.  Used by a number of ship-updating
 *  code segments; so that code isn't duplicated.
 */
void msg_OOF(EntityManager& em, const Ship& s) {
  std::string telegram =
      std::format("{} is out of fuel at {}.", s, prin_ship_orbits(em, s));
  push_telegram(em, s.owner(), s.governor(), telegram);
}

/* followable: returns 1 iff s1 can follow s2 */
bool followable(EntityManager& em, const Ship& s1, const Ship& s2) {
  if (!s2.alive() || !s1.active() || s2.whatorbits() == ScopeLevel::LEVEL_SHIP)
    return false;

  double range = 4.0 * logscale((int)(s1.tech() + 1.0)) * SYSTEMSIZE;

  const auto* r = em.peek_race(s2.owner());
  if (!r) return false;
  /* You can follow your own ships, your allies' ships, or nearby ships */
  return (s1.owner() == s2.owner()) || r->is_allied_with(s1.owner()) ||
         (s1.coordinates().distance_to(s2.coordinates()) <= range);
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
