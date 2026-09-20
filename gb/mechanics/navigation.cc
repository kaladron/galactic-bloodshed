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
    doown(*nested_ship, entity_manager); /* recursive call */
    nested_ship->owner() = ship.owner();
    nested_ship->governor() = ship.governor();
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
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
    telegram << "\t\tNot enough fuel to land!\n";
    return false;
  }
  s.set_land_coords(dest_coords);
  telegram << std::format("\t\tLanded on sector {}\n", s.land_coords());
  const auto& star = *em.peek_star(s.storbits());
  s.set_coordinates(p.absolute_coordinates(star));
  s.consume_fuel(fuel);
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
  const double fuel = s.mass() * p.gravity() * LAUNCH_GRAV_MASS_FACTOR;
  if (s.fuel() < fuel) {
    telegram << "\t\tNot enough fuel to launch!\n";
    return;
  }
  s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
  s.deststar() = route.dest_star;
  s.destpnum() = route.dest_planet;
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

  s.consume_fuel(fuse);
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
  if (!s.speed() || s.docked() || !s.alive()) return false;
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
