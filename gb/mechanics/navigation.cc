// SPDX-License-Identifier: Apache-2.0

/// \file navigation.cc
/// \brief Multi-entity ship movement, merchant routing, and location formatting
/// helpers.

module;

import std;

module gb.mechanics;

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

void collect_captured_ships(EntityManager& em, const Ship& ship,
                            std::vector<CapturedShipEvent>& out) {
  for (auto ship_handle : ShipList::in_carrier(em, ship.number())) {
    Ship& s = *ship_handle;
    s.owner() =
        ship.owner(); /* make sure he gets all of the ships landed on it */
    s.governor() = ship.governor();
    collect_captured_ships(em, s, out); /* recursive call */
    out.push_back(CapturedShipEvent{
        .ship_number = s.number(),
        .ship_display = std::format("{}", s),
        .new_owner = s.owner(),
        .new_governor = s.governor(),
    });
  }
}

}  // namespace

CapturedShipsReport capture_stuff(EntityManager& em, const Ship& ship) {
  std::vector<CapturedShipEvent> captured;
  collect_captured_ships(em, ship, captured);
  return CapturedShipsReport{
      .captured_ships = std::move(captured),
  };
}

void domass(Ship& ship, EntityManager& entity_manager) {
  // Get race mass from EntityManager
  double rmass = 1.0;
  if (ship.owner() != 0) {
    const auto* race = entity_manager.peek_race(ship.owner());
    if (race) {
      rmass = race->mass;
    }
  }

  double carried_mass = 0.0;
  hangar_t carried_hanger = 0;
  for (auto nested_ship : ShipList::in_carrier(entity_manager, ship.number())) {
    domass(*nested_ship, entity_manager); /* recursive call */
    carried_mass += nested_ship->mass();
    carried_hanger += nested_ship->size();
  }
  ship.hanger() = carried_hanger;
  ship.set_mass(ship.local_mass(rmass) + carried_mass);
}

void doown(Ship& ship, EntityManager& entity_manager) {
  for (auto nested_ship : ShipList::in_carrier(entity_manager, ship.number())) {
    nested_ship->owner() = ship.owner();
    nested_ship->governor() = ship.governor();
    doown(*nested_ship, entity_manager); /* recursive call */
  }
}

namespace {

[[nodiscard]] bool merchant_land_ship(EntityManager& em, Ship& s,
                                      const Planet& p,
                                      const Coordinates& dest_coords,
                                      std::stringstream& telegram) {
  if (s.is_landed()) return true;

  const double fuel = s.mass() * p.gravity() * LAND_GRAV_MASS_FACTOR;
  if (s.fuel() < fuel) {
    s.clear_destination();
    telegram << "\t\tNot enough fuel to land!\n";
    return false;
  }
  s.set_land_coords(dest_coords);
  telegram << std::format("\t\tLanded on sector {}\n", s.land_coords());
  const auto& star = *em.peek_star(s.storbits());
  s.set_coordinates(p.absolute_coordinates(star));
  s.consume_fuel(fuel);
  s.land_on_planet();
  return true;
}

void merchant_load_cargo(Ship& s, plinfo& pinfo, const auto& load,
                         std::stringstream& telegram) {
  if (!load.any()) return;

  telegram << "\t\t";
  if (load.fuel) {
    const resource_t amount =
        std::min<resource_t>(s.available_fuel_capacity(), pinfo.fuel);
    pinfo.fuel -= amount;
    s.add_fuel(amount);
    telegram << std::format("{}f ", amount);
  }
  if (load.resources) {
    const resource_t amount =
        std::min<resource_t>(s.available_resource_capacity(), pinfo.resource);
    pinfo.resource -= amount;
    s.add_resource(amount);
    telegram << std::format("{}r ", amount);
  }
  if (load.crystals) {
    const crystal_t amount =
        std::min<crystal_t>(s.available_crystals_capacity(), pinfo.crystals);
    pinfo.crystals -= amount;
    s.add_crystals(amount);
    telegram << std::format("{}x ", amount);
  }
  if (load.destruct) {
    const resource_t amount =
        std::min<resource_t>(s.available_destruct_capacity(), pinfo.destruct);
    pinfo.destruct -= amount;
    s.add_destruct(amount);
    telegram << std::format("{}d ", amount);
  }
  telegram << "loaded\n";
}

void merchant_unload_cargo(Ship& s, plinfo& pinfo, const auto& unload,
                           std::stringstream& telegram) {
  if (!unload.any()) return;

  telegram << "\t\t";
  if (unload.fuel) {
    const resource_t amount = s.fuel_units();
    pinfo.fuel += amount;
    telegram << std::format("{}f ", amount);
    s.consume_fuel(amount);
  }
  if (unload.resources) {
    const resource_t amount = s.resource();
    pinfo.resource += amount;
    telegram << std::format("{}r ", amount);
    s.consume_resource(amount);
  }
  if (unload.crystals) {
    const crystal_t amount = s.crystals();
    pinfo.crystals += amount;
    telegram << std::format("{}x ", amount);
    s.consume_crystals(amount);
  }
  if (unload.destruct) {
    const resource_t amount = s.destruct();
    pinfo.destruct += amount;
    telegram << std::format("{}d ", amount);
    s.consume_destruct(amount);
  }
  telegram << "unloaded\n";
}

void merchant_launch_to_next_stop(EntityManager& em, Ship& s, const Planet& p,
                                  const plroute& route,
                                  std::stringstream& telegram) {
  if (!route.has_destination()) {
    return;
  }
  const double fuel = s.mass() * p.gravity() * LAUNCH_GRAV_MASS_FACTOR;
  if (s.fuel() < fuel) {
    telegram << "\t\tNot enough fuel to launch!\n";
    return;
  }
  s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
  s.set_planet_destination(*route.dest_star, *route.dest_planet);
  s.consume_fuel(fuel);
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
bool do_merchant(EntityManager& em, Ship& s, Planet& p,
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

}  // namespace

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
      return std::format("#{0}", ship.destshipno().value_or(0));
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
      return std::format("#{0}", ship.destshipno().value_or(0));
    case ScopeLevel::LEVEL_UNIV:
      return "/";
  }
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
      if (s.destshipno()) {
        if (const auto* mothership = em.peek_ship(*s.destshipno())) {
          return prin_ship_orbits(em, *mothership);
        }
      }
      return "/";
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
      return std::format("#{}", ship.destshipno().value_or(0));
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

  if (s.docked() || !s.has_celestial_destination()) {
    s.hyper_drive().on = false;
    return;
  }

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

  s.consume_fuel(fuse);
  const double heading =
      std::atan2(dest_star->coordinates().x - s.coordinates().x,
                 dest_star->coordinates().y - s.coordinates().y);
  const double sn = std::sin(heading);
  const double cs = std::cos(heading);
  s.set_coordinates(
      UniverseCoordinates{dest_star->coordinates().x - sn * 0.9 * SYSTEMSIZE,
                          dest_star->coordinates().y - cs * 0.9 * SYSTEMSIZE});
  s.enter_star_orbit(s.deststar());
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
      s.enter_star_orbit();
      s.protect().planet = false;
    }
  } else if (s.whatorbits() == ScopeLevel::LEVEL_STAR) {
    const auto* ost = em.peek_star(s.storbits());
    if (ost && s.coordinates().distance_to(ost->coordinates()) > SYSTEMSIZE) {
      s.enter_deep_space();
      s.protect().evade = false;
      s.protect().planet = false;
    }
  }
}

void execute_navigation_step(EntityManager& em, Ship& s, double fuse,
                             double mfactor) {
  const double heading = DEGREES_TO_RADIANS * s.navigate().bearing;
  s.consume_fuel(fuse);
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
  res.target_ship = s.destshipno() ? em.peek_ship(*s.destshipno()) : nullptr;
  if (!res.target_ship) {
    s.clear_destination();
    s.protect().evade = false;
    res.valid = false;
    return;
  }
  s.sync_followed_ship_orbit(*res.target_ship);
  res.star = s.deststar();
  res.pnum = s.destpnum();
  res.coords = res.target_ship->coordinates();

  const ScopeLevel dsh_orbit = res.target_ship->whatorbits();
  if (dsh_orbit == ScopeLevel::LEVEL_PLAN &&
      (s.whatorbits() != ScopeLevel::LEVEL_PLAN ||
       s.storbits() != res.target_ship->storbits() ||
       s.pnumorbits() != res.target_ship->pnumorbits())) {
    res.level = ScopeLevel::LEVEL_PLAN;
  } else if (dsh_orbit == ScopeLevel::LEVEL_STAR &&
             (s.whatorbits() == ScopeLevel::LEVEL_UNIV ||
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

  s.enter_star_orbit(deststar);
  s.protect().planet = false;
  if (can_ship_explore(s, checking_fuel)) {
    explore_arrived_star(em, s, deststar);
  }
  if (send_messages && s.type() != ShipType::OTYPE_VN) {
    push_telegram(em, s.owner(), s.governor(),
                  std::format("{} arrived at {}.", s, prin_ship_orbits(em, s)));
  }
  if (s.whatdest() == ScopeLevel::LEVEL_STAR) {
    s.clear_destination();
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
      s.clear_destination();
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
  s.enter_planet_orbit(destpnum);

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
    s.enter_planet_orbit(dsh.storbits(), dsh.pnumorbits());
  } else if (dsh.whatorbits() == ScopeLevel::LEVEL_STAR) {
    s.enter_star_orbit(dsh.storbits());
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
    const auto lost_dest = s.destshipno().value_or(0);
    s.clear_destination();
    s.protect().evade = false;
    if (send_messages) {
      push_telegram(em, s.owner(), s.governor(),
                    std::format("{} at {} lost sight of destination ship #{}.",
                                s, prin_ship_orbits(em, s), lost_dest));
    }
    return;
  }

  if (truedist > DIST_TO_LAND) {
    s.consume_fuel(fuse);
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

[[nodiscard]] bool can_ship_move_sublight(const Ship& s) noexcept {
  if (!s.speed() || s.docked() || !s.alive() ||
      s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    return false;
  }
  return s.whatdest() != ScopeLevel::LEVEL_UNIV || s.navigate().on;
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

  // Normal sublight cruise burns 0.5x the fuel rate; evasive maneuvers
  // double fuel consumption to the full 1.0x rate.
  const double evade_multiplier = s.protect().evade ? 1.0 : 0.5;
  const double fuse = evade_multiplier * s.speed() * s.mass() * FUEL_USE /
                      static_cast<double>(state->segments);
  if (s.fuel() < fuse) {
    handle_sublight_out_of_fuel(em, s, send_messages);
    return;
  }

  const double mfactor = s.sublight_move_factor(state->segments);
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

namespace {
std::expected<int, JettisonError> resolve_jettison_amount(int requested_amount,
                                                          int max_available) {
  const int amount = (requested_amount == 0) ? max_available : requested_amount;
  if (amount < 0) {
    return std::unexpected(
        JettisonError{.reason = JettisonErrorReason::NegativeAmount});
  }
  if (amount > max_available) {
    return std::unexpected(JettisonError{
        .reason = JettisonErrorReason::ExceedsAvailable,
        .max_available = max_available,
    });
  }
  if (amount == 0) {
    return std::unexpected(
        JettisonError{.reason = JettisonErrorReason::NothingToJettison});
  }
  return amount;
}
}  // namespace

std::expected<JettisonResult, JettisonError>
jettison_ship_cargo(Ship& ship, char commod, int requested_amount,
                    double race_mass) {
  if (ship.is_landed()) {
    return std::unexpected(
        JettisonError{.reason = JettisonErrorReason::ShipLanded});
  }
  if (!ship.active()) {
    return std::unexpected(JettisonError{
        .reason = JettisonErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", ship),
    });
  }

  switch (commod) {
    case 'x': {
      auto amt = resolve_jettison_amount(requested_amount,
                                         static_cast<int>(ship.crystals()));
      if (!amt) return std::unexpected(amt.error());
      ship.consume_crystals(*amt);
      return JettisonResult{
          .commodity = JettisonCommodity::Crystals,
          .amount = *amt,
      };
    }
    case 'c': {
      auto amt = resolve_jettison_amount(requested_amount,
                                         static_cast<int>(ship.popn()));
      if (!amt) return std::unexpected(amt.error());
      ship.remove_popn(*amt, race_mass);
      return JettisonResult{
          .commodity = JettisonCommodity::Crew,
          .amount = *amt,
          .ship_display = std::format("{}", ship),
          .remaining_complement = ship.popn(),
      };
    }
    case 'm': {
      auto amt = resolve_jettison_amount(requested_amount,
                                         static_cast<int>(ship.troops()));
      if (!amt) return std::unexpected(amt.error());
      ship.remove_troops(*amt, race_mass);
      return JettisonResult{
          .commodity = JettisonCommodity::Military,
          .amount = *amt,
          .ship_number = ship.number(),
          .remaining_complement = ship.troops(),
      };
    }
    case 'd': {
      auto amt = resolve_jettison_amount(requested_amount,
                                         static_cast<int>(ship.destruct()));
      if (!amt) return std::unexpected(amt.error());
      ship.consume_destruct(*amt);
      return JettisonResult{
          .commodity = JettisonCommodity::Destruct,
          .amount = *amt,
          .ship_display = std::format("{}", ship),
          .check_boobytrap = !ship.max_crew_capacity(),
          .still_boobytrapped = ship.destruct() > 0,
      };
    }
    case 'f': {
      auto amt = resolve_jettison_amount(requested_amount,
                                         static_cast<int>(ship.fuel()));
      if (!amt) return std::unexpected(amt.error());
      ship.consume_fuel(static_cast<double>(*amt));
      return JettisonResult{
          .commodity = JettisonCommodity::Fuel,
          .amount = *amt,
      };
    }
    case 'r': {
      auto amt = resolve_jettison_amount(requested_amount,
                                         static_cast<int>(ship.resource()));
      if (!amt) return std::unexpected(amt.error());
      ship.consume_resource(*amt);
      return JettisonResult{
          .commodity = JettisonCommodity::Resources,
          .amount = *amt,
      };
    }
    default:
      return std::unexpected(
          JettisonError{.reason = JettisonErrorReason::InvalidCommodity});
  }
}

std::expected<MountCrystalResult, MountCrystalError>
mount_ship_crystal(Ship& ship) {
  if (!ship.mount()) {
    return std::unexpected(MountCrystalError::NoCrystalMount);
  }
  if (ship.mounted()) {
    return std::unexpected(MountCrystalError::AlreadyMounted);
  }
  if (!ship.crystals()) {
    return std::unexpected(MountCrystalError::NoCrystalsOnBoard);
  }
  ship.mounted() = 1;
  ship.consume_crystals(1);
  return MountCrystalResult{};
}

std::expected<DismountCrystalResult, DismountCrystalError>
dismount_ship_crystal(Ship& ship) {
  if (!ship.mount()) {
    return std::unexpected(DismountCrystalError::NoCrystalMount);
  }
  if (!ship.mounted()) {
    return std::unexpected(DismountCrystalError::NotMounted);
  }
  if (ship.crystals() >= ship.max_crystals_capacity()) {
    return std::unexpected(DismountCrystalError::MaxCrystalsOnBoard);
  }
  ship.mounted() = 0;
  ship.add_crystals(1);
  const bool discharged = ship.hyper_drive().charge > 0;
  if (discharged) {
    ship.hyper_drive().charge = 0;
  }
  const bool deactivated = ship.laser() && ship.fire_laser();
  if (deactivated) {
    ship.fire_laser() = 0;
  }
  return DismountCrystalResult{
      .hyperdrive_discharged = discharged,
      .laser_deactivated = deactivated,
  };
}

GrantShipResult grant_ship_governor(EntityManager& em, Ship& ship,
                                    governor_t new_gov,
                                    std::string_view donor_gov_name,
                                    std::string_view recipient_gov_name) {
  ship.governor() = new_gov;
  return GrantShipResult{
      .ship_display = std::format("{}", ship),
      .orbits_display = prin_ship_orbits(em, ship),
      .donor_governor_name = std::string(donor_gov_name),
      .recipient_governor_name = std::string(recipient_gov_name),
  };
}

CargoTransferLimits
compute_planet_transfer_limits(EntityManager& em, const Ship& s,
                               player_t player, ShipCargoType cargo,
                               starnum_t snum, planetnum_t pnum) {
  CargoTransferLimits lim{};
  switch (cargo) {
    case ShipCargoType::Crystal:
      em.with_planet(snum, pnum, [&](const Planet& p) {
        lim.uplim = std::min<std::int64_t>(p.info(player).crystals,
                                           s.available_crystals_capacity());
        lim.lolim = -s.crystals();
      });
      break;
    case ShipCargoType::Crew:
      em.with_sectormap(snum, pnum, [&](const SectorMap& smap) {
        const auto& sect = smap.get(s.land_coords());
        lim.uplim = std::min<std::int64_t>(sect.get_popn(),
                                           s.available_crew_capacity());
        lim.lolim = -s.popn();
      });
      break;
    case ShipCargoType::Troops:
      em.with_sectormap(snum, pnum, [&](const SectorMap& smap) {
        const auto& sect = smap.get(s.land_coords());
        lim.uplim = std::min<std::int64_t>(sect.get_troops(),
                                           s.available_crew_capacity());
        lim.lolim = -s.troops();
      });
      break;
    case ShipCargoType::Destruct:
      em.with_planet(snum, pnum, [&](const Planet& p) {
        lim.uplim = std::min<std::int64_t>(p.info(player).destruct,
                                           s.available_destruct_capacity());
        lim.lolim = -s.destruct();
      });
      break;
    case ShipCargoType::Fuel:
      em.with_planet(snum, pnum, [&](const Planet& p) {
        lim.uplim = std::min<std::int64_t>(p.info(player).fuel,
                                           s.available_fuel_capacity());
        lim.lolim = -s.fuel_units();
      });
      break;
    case ShipCargoType::Resource:
      em.with_planet(snum, pnum, [&](const Planet& p) {
        if (s.can_strap_cargo_to_hull()) {
          lim.uplim = p.info(player).resource;
        } else {
          lim.uplim = std::min<std::int64_t>(p.info(player).resource,
                                             s.available_resource_capacity());
        }
        lim.lolim = -s.resource();
      });
      break;
  }
  return lim;
}

namespace {

AlienSectorAssaultResult
unload_onto_alien_sector(EntityManager& em, Planet& planet, SectorMap& smap,
                         Ship& ship, Sector& sect, PopulationType what,
                         population_t people, player_t attacker_player,
                         governor_t attacker_gov, starnum_t snum,
                         planetnum_t pnum) {
  AlienSectorAssaultResult res{
      .attacker_type = what,
      .attacker_player = attacker_player,
      .attacker_governor = attacker_gov,
      .ship_display = std::format("{}", ship),
      .sector_coords = ship.land_coords(),
      .sector_condition_symbol = sect.condition_symbol(),
  };

  if (people <= 0) {
    res.requires_unload = true;
    return res;
  }

  const player_t defender_owner = sect.get_owner();
  res.defender_player = defender_owner;
  res.unloaded_people = people;

  em.mutate_race(attacker_player, [&](Race& race) {
    em.mutate_race(defender_owner, [&](Race& alien) {
      em.mutate_star(snum,
                     [&](Star& s) { s.record_ground_assault(race, alien); });

      alien.increase_translation(attacker_player);
      race.increase_translation(defender_owner);

      const auto& star = *em.peek_star(snum);
      const governor_t defender_gov = star.governor(defender_owner);
      res.defender_governor = defender_gov;
      res.attacker_race_name = race.name;
      res.defender_race_name = alien.name;
      res.star_name = star.get_name();
      res.planet_name = star.get_planet_name(pnum);

      if (what == PopulationType::CIV) {
        ship.remove_popn(people, race.mass);
      } else {
        ship.remove_troops(people, race.mass);
      }
      res.ship_civ_after_unload = ship.popn();
      res.ship_mil_after_unload = ship.troops();

      const population_t initial_attacker_popn = people;
      const population_t initial_defender_civ = sect.get_popn();
      const population_t initial_defender_mil = sect.get_troops();
      res.initial_defender_civ = initial_defender_civ;
      res.initial_defender_mil = initial_defender_mil;

      const auto outcome = ground_attack({
          .attacker = race,
          .defender = alien,
          .attacker_force = people,
          .attacker_type = what,
          .defender_civ = initial_defender_civ,
          .defender_mil = initial_defender_mil,
          .attacker_defense_bonus = static_cast<int>(ship.armor()),
          .defender_defense_bonus = sect.defense_bonus(),
          .attacker_compatibility =
              percent_to_fraction(100.0 - static_cast<double>(ship.damage())),
          .defender_compatibility = alien.sector_compatibility(sect),
      });
      const population_t surviving = outcome.surviving_attackers;
      sect.set_popn_exact(outcome.surviving_defender_civ);
      sect.set_troops(outcome.surviving_defender_mil);

      res.attack_strength = outcome.attack_strength;
      res.defense_strength = outcome.defense_strength;
      res.surviving_attackers = surviving;
      res.attacker_casualties = outcome.attacker_casualties;
      res.defender_civ_casualties = outcome.defender_civ_casualties;
      res.defender_mil_casualties = outcome.defender_mil_casualties;

      if (sect.is_empty()) {
        res.defender_wiped_out = true;
        int absorbed = 0;
        if (race.absorb) {
          res.attacker_absorbed_bodies = true;
          absorbed = int_rand(0, initial_defender_civ + initial_defender_mil);
          res.absorbed_bodies = absorbed;
        }
        if (what == PopulationType::CIV) {
          sect.set_popn_exact(surviving + absorbed);
        } else {
          sect.set_popn_exact(absorbed);
          sect.set_troops(surviving);
        }
        if (!sect.is_empty()) {
          sect.set_owner(attacker_player);
          race.adjust_morale(alien, static_cast<int>(alien.fighters));
        } else {
          sect.clear_owner();
        }
      } else {
        res.defender_wiped_out = false;
        if (alien.absorb) {
          res.defender_absorbed_bodies = true;
          const int absorbed = int_rand(0, initial_attacker_popn - surviving);
          res.absorbed_bodies = absorbed;
          sect.add_popn(absorbed);
        }
        if (what == PopulationType::CIV) {
          ship.add_popn(surviving, race.mass);
        } else {
          ship.add_troops(surviving, race.mass);
        }
        alien.adjust_morale(race, static_cast<int>(race.fighters));
      }

      res.attacker_won = (sect.get_owner() == attacker_player);
    });
  });

  planet.sync_demographics(smap);
  return res;
}

}  // namespace

PlanetCargoTransferResult
execute_planet_cargo_transfer(EntityManager& em, Ship& s, player_t player,
                              governor_t governor, starnum_t snum,
                              planetnum_t pnum, ShipCargoType cargo,
                              std::int64_t amt, double race_mass) {
  PlanetCargoTransferResult result{
      .commodity = cargo,
      .amount = amt,
      .ship_display = std::format("{}", s),
      .sector_coords = s.land_coords(),
  };

  em.mutate_planet_and_sectors(snum, pnum, [&](Planet& p, SectorMap& smap) {
    auto& sect = smap.get(s.land_coords());
    switch (cargo) {
      case ShipCargoType::Crew:
      case ShipCargoType::Troops: {
        const PopulationType what = (cargo == ShipCargoType::Crew)
                                        ? PopulationType::CIV
                                        : PopulationType::MIL;
        if (sect.get_owner() != 0 && sect.get_owner() != player) {
          result.assault = unload_onto_alien_sector(
              em, p, smap, s, sect, what, -amt, player, governor, snum, pnum);
          return;
        }
        const bool was_empty = sect.is_empty();
        const population_t civ_delta =
            (cargo == ShipCargoType::Crew) ? -amt : 0;
        const population_t mil_delta =
            (cargo == ShipCargoType::Troops) ? -amt : 0;
        p.adjust_sector_population(sect, player, civ_delta, mil_delta);
        const bool is_now_empty = sect.is_empty();

        if (was_empty && !is_now_empty && amt < 0) {
          result.sector_event = (cargo == ShipCargoType::Crew)
                                    ? PlanetSectorEvent::Colonized
                                    : PlanetSectorEvent::Occupied;
        } else if (!was_empty && is_now_empty) {
          result.sector_event = PlanetSectorEvent::Evacuated;
        }
        break;
      }
      case ShipCargoType::Destruct:
        p.info(player).destruct -= amt;
        break;
      case ShipCargoType::Crystal:
        p.info(player).crystals -= amt;
        break;
      case ShipCargoType::Fuel:
        p.info(player).fuel -= amt;
        break;
      case ShipCargoType::Resource:
        p.info(player).resource -= amt;
        break;
    }
  });

  if (result.assault.has_value()) {
    result.resulting_popn = s.popn();
    result.resulting_troops = s.troops();
    return result;
  }

  switch (cargo) {
    case ShipCargoType::Crew:
      s.add_popn(amt, race_mass);
      break;
    case ShipCargoType::Troops:
      s.add_troops(amt, race_mass);
      break;
    case ShipCargoType::Destruct:
      s.add_destruct(amt);
      break;
    case ShipCargoType::Crystal:
      s.add_crystals(amt);
      break;
    case ShipCargoType::Fuel:
      s.add_fuel(static_cast<double>(amt));
      break;
    case ShipCargoType::Resource:
      s.add_resource(amt);
      break;
  }

  result.resulting_popn = s.popn();
  result.resulting_troops = s.troops();
  result.check_boobytrap = !s.max_crew_capacity();
  result.is_boobytrapped = s.destruct() > 0;
  return result;
}

std::expected<ShipDockingStatus, ShipDockingError>
validate_ship_docking(EntityManager& em, Ship& s, player_t player,
                      ScopeLevel scope_level, starnum_t snum,
                      planetnum_t pnum) {
  if (!s.active()) {
    return std::unexpected(ShipDockingError{
        .reason = ShipDockingErrorReason::Inactive,
        .ship_display = std::format("{}", s),
    });
  }
  if (!s.docked()) {
    return std::unexpected(ShipDockingError{
        .reason = ShipDockingErrorReason::NotLandedOrDocked,
        .ship_display = std::format("{}", s),
    });
  }

  if (s.whatdest() == ScopeLevel::LEVEL_PLAN) {
    if (scope_level != ScopeLevel::LEVEL_PLAN || s.storbits() != snum ||
        s.pnumorbits() != pnum) {
      return std::unexpected(ShipDockingError{
          .reason = ShipDockingErrorReason::WrongPlanetScope,
          .ship_display = std::format("{}", s),
          .land_coords = s.land_coords(),
      });
    }
    return ShipDockingStatus{
        .is_docked_to_ship = false,
        .is_different_owner = false,
        .ship_display = std::format("{}", s),
        .land_coords = s.land_coords(),
    };
  }

  if (!s.destshipno()) {
    return std::unexpected(ShipDockingError{
        .reason = ShipDockingErrorReason::NotDocked,
        .ship_display = std::format("{}", s),
    });
  }

  std::optional<std::expected<ShipDockingStatus, ShipDockingError>> outcome;
  try {
    em.with_ship(*s.destshipno(), [&](const Ship& s2) {
      const bool mutually_docked = (s.whatorbits() == ScopeLevel::LEVEL_SHIP ||
                                    s2.destshipno() == s.number());
      if (!s2.alive() || !mutually_docked) {
        s.launch_to_orbit(s.whatorbits());
        outcome = std::unexpected(ShipDockingError{
            .reason = ShipDockingErrorReason::TargetNotDocked,
            .ship_display = std::format("{}", s),
            .target_ship_display = std::format("{}", s2),
        });
        return;
      }
      if (s2.is_overloaded() && s2.whatorbits() == ScopeLevel::LEVEL_SHIP) {
        outcome = std::unexpected(ShipDockingError{
            .reason = ShipDockingErrorReason::TargetOverloaded,
            .ship_display = std::format("{}", s),
            .target_ship_display = std::format("{}", s2),
        });
        return;
      }
      const bool diff = (s2.owner() != player);
      outcome = ShipDockingStatus{
          .is_docked_to_ship = true,
          .is_different_owner = diff,
          .ship_display = std::format("{}", s),
          .target_ship_display = std::format("{}", s2),
          .target_owner = s2.owner(),
      };
    });
  } catch (const EntityNotFoundError&) {
    return std::unexpected(ShipDockingError{
        .reason = ShipDockingErrorReason::DestinationBogus,
        .ship_display = std::format("{}", s),
    });
  }
  return *outcome;
}

CargoTransferLimits compute_ship_transfer_limits(const Ship& s, const Ship& s2,
                                                 ShipCargoType cargo,
                                                 bool diff) {
  CargoTransferLimits lim{};
  switch (cargo) {
    case ShipCargoType::Crystal:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(
                             s2.crystals(), s.available_crystals_capacity());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(
                             s.crystals(), s2.available_crystals_capacity());
      break;
    case ShipCargoType::Crew:
      lim.uplim =
          diff ? 0
               : std::min<std::int64_t>(s2.popn(), s.available_crew_capacity());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(s.popn(),
                                                 s2.available_crew_capacity());
      break;
    case ShipCargoType::Troops:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(s2.troops(),
                                                s.available_crew_capacity());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(s.troops(),
                                                 s2.available_crew_capacity());
      break;
    case ShipCargoType::Destruct:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(
                             s2.destruct(), s.available_destruct_capacity());
      lim.lolim = -std::min<std::int64_t>(s.destruct(),
                                          s2.available_destruct_capacity());
      break;
    case ShipCargoType::Fuel:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(s2.fuel_units(),
                                                s.available_fuel_capacity());
      lim.lolim =
          -std::min<std::int64_t>(s.fuel_units(), s2.available_fuel_capacity());
      break;
    case ShipCargoType::Resource:
      if (s.can_strap_cargo_to_hull()) {
        lim.uplim = diff ? 0 : s2.resource();
      } else {
        lim.uplim = diff ? 0
                         : std::min<std::int64_t>(
                               s2.resource(), s.available_resource_capacity());
      }
      if (s2.can_strap_cargo_to_hull()) {
        lim.lolim = -s.resource();
      } else {
        lim.lolim = -std::min<std::int64_t>(s.resource(),
                                            s2.available_resource_capacity());
      }
      break;
  }
  return lim;
}

ShipCargoTransferResult
execute_ship_to_ship_cargo_transfer(EntityManager& em, Ship& s,
                                    ShipCargoType cargo, std::int64_t amt,
                                    double race_mass, bool is_metamorph) {
  ShipCargoTransferResult result{
      .commodity = cargo,
      .amount = amt,
      .ship_display = std::format("{}", s),
      .is_metamorph = is_metamorph,
  };

  if (s.destshipno()) {
    em.mutate_ship(*s.destshipno(), [&](Ship& s2) {
      if (amt > 0) {
        s2.transfer_cargo_to(s, cargo, amt, race_mass);
      } else {
        s.transfer_cargo_to(s2, cargo, -amt, race_mass);
      }
      result.target_ship_display = std::format("{}", s2);
      result.target_owner = s2.owner();
      result.target_governor = s2.governor();
      result.notify_target_owner = (s2.owner() != s.owner() && amt < 0);
    });
  }

  result.resulting_popn = s.popn();
  result.resulting_troops = s.troops();
  result.check_boobytrap = !s.max_crew_capacity();
  result.is_boobytrapped = s.destruct() > 0;
  return result;
}

std::expected<TransporterTransferResult, TransporterError>
execute_transporter_transfer(EntityManager& em, TransporterShip& s,
                             starnum_t snum, planetnum_t pnum, double race_mass,
                             bool is_metamorph) {
  if (!s.is_landed()) {
    return std::unexpected(TransporterError::OriginNotLanded);
  }
  if (s.storbits() != snum || s.pnumorbits() != pnum) {
    return std::unexpected(TransporterError::WrongPlanetScope);
  }
  if (s.damage()) {
    return std::unexpected(TransporterError::OriginDamaged);
  }
  if (!s.target_ship()) {
    return std::unexpected(TransporterError::HopperBlocked);
  }

  std::optional<std::expected<TransporterTransferResult, TransporterError>>
      outcome;
  try {
    em.mutate_ship(*s.target_ship(), [&](Ship& s2) {
      if (!s2.alive() || s2.type() != ShipType::OTYPE_TRANSDEV || !s2.on()) {
        outcome = std::unexpected(TransporterError::TargetNotReceiving);
        return;
      }
      if (!s2.is_landed()) {
        outcome = std::unexpected(TransporterError::TargetNotLanded);
        return;
      }
      if (s2.damage()) {
        outcome = std::unexpected(TransporterError::TargetDamaged);
        return;
      }

      TransporterTransferResult res{
          .sender_ship_display = std::format("{}", static_cast<const Ship&>(s)),
          .receiver_ship_display = std::format("{}", s2),
          .receiver_owner = s2.owner(),
          .receiver_governor = s2.governor(),
          .notify_receiver = (s2.owner() != s.owner()),
          .is_metamorph = is_metamorph,
      };

      const auto try_beam = [&](ShipCargoType type, std::int64_t amount,
                                double mass) {
        if (amount <= 0) return;
        const auto transferred = s.transfer_cargo_to(s2, type, amount, mass);
        if (transferred > 0) {
          res.transfers.push_back(TransporterCommodityTransfer{
              .commodity = type,
              .amount = transferred,
          });
        }
      };

      try_beam(ShipCargoType::Resource, s.resource(), 0.0);
      try_beam(ShipCargoType::Fuel, s.fuel_units(), 0.0);
      try_beam(ShipCargoType::Destruct, s.destruct(), 0.0);
      try_beam(ShipCargoType::Crew, s.popn(), race_mass);
      try_beam(ShipCargoType::Troops, s.troops(), race_mass);
      try_beam(ShipCargoType::Crystal, s.crystals(), 0.0);

      outcome = std::move(res);
    });
  } catch (const EntityNotFoundError&) {
    return std::unexpected(TransporterError::HopperBlocked);
  }
  return *outcome;
}
