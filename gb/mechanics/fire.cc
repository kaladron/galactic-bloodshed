// SPDX-License-Identifier: Apache-2.0

/// \file fire.cc
/// \brief Fire at ship or planet from ship or planet

module;

import std;

module gb.mechanics;

/**
 * @brief Checks to see if there are any planetary defense networks on the
 * planet.
 *
 * @param star_id The star id.
 * @param planet_order The planet order.
 * @param Playernum The player number.
 * @return True if there are planetary defense networks, false otherwise.
 */
bool has_planet_defense(EntityManager& entity_manager, const starnum_t star_id,
                        const planetnum_t planet_order,
                        const player_t Playernum) {
  for (const Ship& s :
       ShipList::readonly_on_planet(entity_manager, star_id, planet_order)) {
    if (s.type() == ShipType::OTYPE_PLANDEF && s.owner() != Playernum) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Checks for overload conditions on a ship's crystal and handles the
 * consequences.
 *
 * @param ship The ship object to check for overload conditions.
 * @param cew Strength of Confined Energy Weapons.
 * @param strength The requested weapon strength of the ship.
 * @return Pair of effective weapon strength (0 on overload) and optional
 * ReactorOverloadEvent if crystal burnout or explosion occurs.
 */
std::pair<weapon_power_t, std::optional<ReactorOverloadEvent>>
check_overload(EntityManager& entity_manager, Ship& ship, int cew,
               weapon_power_t strength) {
  if (!ship.is_laser_on() && (cew == 0)) {
    return {strength, std::nullopt};
  }

  // Check to see if the ship blows up
  if (uniform_rand(0, strength) >
      static_cast<int>((1.0 - .01 * ship.damage()) * ship.tech() / 2.0)) {
    ReactorOverloadEvent event{
        .outcome = ReactorOverloadOutcome::ShipExploded,
        .owner = ship.owner(),
        .governor = ship.governor(),
        .scope = ship.whatorbits(),
        .star_id = ship.storbits(),
        .location_display = dispshiploc(entity_manager, ship),
        .ship_display = std::format("{}", ship),
    };
    entity_manager.kill_ship(ship.owner(), ship);
    return {0, event};
  }
  if (uniform_rand(0, strength) >
      static_cast<int>((1.0 - .01 * ship.damage()) * ship.tech() / 4.0)) {
    ship.fire_laser() = 0;
    ship.mounted() = 0;
    return {
        0,
        ReactorOverloadEvent{
            .outcome = ReactorOverloadOutcome::CrystalDamaged,
            .owner = ship.owner(),
            .governor = ship.governor(),
            .scope = ship.whatorbits(),
            .star_id = ship.storbits(),
            .location_display = dispshiploc(entity_manager, ship),
            .ship_display = std::format("{}", ship),
        },
    };
  }
  return {strength, std::nullopt};
}

namespace {

bool is_eligible_escort(const Ship& escort, shipnum_t protected_id,
                        shipnum_t shooter_id) {
  if (!escort.active() || !escort.protect().on) {
    return false;
  }
  if (escort.protect().ship != protected_id) {
    return false;
  }
  return escort.number() != protected_id && escort.number() != shooter_id;
}

void resolve_target_self_retaliation(EntityManager& em, Ship& target_ship,
                                     Ship& shooter_ship, weapon_power_t retal,
                                     damage_t damage,
                                     ShipCombatExchange& exchange) {
  if (retal <= 0 || damage <= 0 || !target_ship.protect().retaliate) {
    return;
  }
  auto [retal_strength, overload] = check_overload(em, target_ship, 0, retal);
  if (overload) {
    exchange.retaliation_overload = overload;
    return;
  }
  exchange.retaliation_shot = shoot_ship_to_ship(em, target_ship, shooter_ship,
                                                 retal_strength, 0, true);
  if (exchange.retaliation_shot) {
    target_ship.consume_weapon_resources(retal_strength);
  }
}

std::optional<EscortRetaliationEvent>
execute_escort_shot(EntityManager& em, Ship& escort, Ship& shooter_ship) {
  auto [strength, overload] =
      check_overload(em, escort, 0, escort.check_retal_strength());
  if (overload) {
    return EscortRetaliationEvent{
        .escort_owner = escort.owner(),
        .escort_governor = escort.governor(),
        .overload = overload,
    };
  }
  auto shot = shoot_ship_to_ship(em, escort, shooter_ship, strength, 0);
  if (!shot) {
    return std::nullopt;
  }
  escort.consume_weapon_resources(strength);
  return EscortRetaliationEvent{
      .escort_owner = escort.owner(),
      .escort_governor = escort.governor(),
      .shot = shot,
  };
}

void resolve_escort_retaliation(EntityManager& em, const Ship& protected_ship,
                                Ship& shooter_ship, damage_t damage,
                                ShipCombatExchange& exchange) {
  if (damage <= 0 || !shooter_ship.alive() ||
      shooter_ship.type() == ShipType::OTYPE_AFV) {
    return;
  }

  ShipList shiplist = (protected_ship.whatorbits() == ScopeLevel::LEVEL_STAR)
                          ? ShipList::in_star(em, protected_ship.storbits())
                          : ShipList::on_planet(em, protected_ship.storbits(),
                                                protected_ship.pnumorbits());
  for (auto ship_handle : shiplist) {
    if (!shooter_ship.alive()) {
      break;
    }
    Ship& escort = *ship_handle;
    if (!is_eligible_escort(escort, protected_ship.number(),
                            shooter_ship.number())) {
      continue;
    }
    if (auto event = execute_escort_shot(em, escort, shooter_ship)) {
      exchange.escort_shots.push_back(*event);
    }
  }
}

std::optional<FireError> validate_surface_combat_geometry(EntityManager& em,
                                                          const Ship& from,
                                                          const Ship& to) {
  if (from.type() == ShipType::OTYPE_AFV) {
    if (!from.is_landed()) {
      return FireError{
          .reason = FireErrorReason::AfvNotLanded,
          .ship_display = std::format("{}", from),
      };
    }
    if (!to.is_landed()) {
      return FireError{
          .reason = FireErrorReason::AfvTargetNotLanded,
          .target_display = std::format("{}", to),
      };
    }
  }

  if (!from.is_landed() || !to.is_landed()) {
    return std::nullopt;
  }
  if (from.storbits() != to.storbits() ||
      from.pnumorbits() != to.pnumorbits()) {
    return FireError{.reason = FireErrorReason::LandedOnDifferentPlanets};
  }
  const auto& p = *em.peek_planet(from.storbits(), from.pnumorbits());
  if (!p.is_adjacent(from.land_coords(), to.land_coords())) {
    return FireError{.reason = FireErrorReason::NotAdjacentOnPlanet};
  }
  return std::nullopt;
}

std::optional<FireError> validate_cew_fire(const Ship& from, const Ship& to) {
  if (!from.cew()) {
    return FireError{.reason = FireErrorReason::NotEquippedForCew};
  }
  if (!from.mounted()) {
    return FireError{.reason = FireErrorReason::NoCrystalMounted};
  }
  if (from.fuel() < static_cast<double>(from.cew())) {
    return FireError{
        .reason = FireErrorReason::InsufficientCewFuel,
        .cew_strength = from.cew(),
    };
  }
  if (from.is_landed() || to.is_landed()) {
    return FireError{.reason = FireErrorReason::CewLandedOriginOrTarget};
  }
  return std::nullopt;
}

std::optional<FireErrorReason>
check_fire_ap(EntityManager& em, const Ship& from, player_t player, bool god) {
  if (god) {
    return std::nullopt;
  }
  if (from.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (em.peek_universe()->get_AP(player) < 1) {
      return FireErrorReason::InsufficientUniverseAp;
    }
    return std::nullopt;
  }
  if (em.peek_star(from.storbits())->AP(player) < 1) {
    return FireErrorReason::InsufficientStarAp;
  }
  return std::nullopt;
}

void deduct_fire_ap(EntityManager& em, const Ship& from, player_t player,
                    bool god) {
  if (god) {
    return;
  }
  if (from.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    em.mutate_universe([&](Universe& u) { u.deduct_AP(player, 1); });
    return;
  }
  em.mutate_star(from.storbits(), [&](Star& s) { s.AP(player) -= 1; });
}

std::optional<FireErrorReason>
execute_ship_attack(EntityManager& em, Ship& from, shipnum_t target_id,
                    weapon_power_t initial_strength, bool is_cew,
                    player_t player, bool god, FireShipResult& result) {
  if (auto ap_err = check_fire_ap(em, from, player, god)) {
    return ap_err;
  }
  if (initial_strength <= 0) {
    return FireErrorReason::NoAttackStrength;
  }

  const int cew_range_flag = is_cew ? 1 : 0;
  const Ship& to_peek = *em.peek_ship(target_id);
  result.exchange = ShipCombatExchange{
      .shooter_owner = from.owner(),
      .shooter_governor = from.governor(),
      .target_owner = to_peek.owner(),
      .target_governor = to_peek.governor(),
      .star_id = from.storbits(),
  };

  auto [strength, overload] =
      check_overload(em, from, cew_range_flag, initial_strength);
  if (overload) {
    result.exchange.primary_overload = overload;
    result.exchange.primary_overload_fizzled = true;
    deduct_fire_ap(em, from, player, god);
    return std::nullopt;
  }

  const auto retal = to_peek.check_retal_strength();
  bool fired = false;

  em.mutate_ship(target_id, [&](Ship& to_ship) {
    result.exchange.primary_shot =
        shoot_ship_to_ship(em, from, to_ship, strength, cew_range_flag);
    if (!result.exchange.primary_shot) {
      return;
    }
    fired = true;
    const damage_t damage = result.exchange.primary_shot->damage;
    from.consume_weapon_resources(strength, is_cew);
    resolve_target_self_retaliation(em, to_ship, from, retal, damage,
                                    result.exchange);
    resolve_escort_retaliation(em, to_ship, from, damage, result.exchange);
  });

  if (!fired) {
    return FireErrorReason::IllegalAttack;
  }

  deduct_fire_ap(em, from, player, god);
  return std::nullopt;
}

std::expected<Coordinates, BombardErrorReason>
resolve_bombard_coords(EntityManager& em, const Ship& from, const Planet& p,
                       std::optional<std::string_view> coords_arg,
                       player_t player) {
  if (!from.is_landed() &&
      has_planet_defense(em, p.star_id(), p.planet_order(), player)) {
    return std::unexpected(BombardErrorReason::PlanetaryDefenseNetworksPresent);
  }
  Coordinates target_coords{};
  if (coords_arg) {
    auto coords_opt = Coordinates::parse(*coords_arg);
    if (!coords_opt) {
      return std::unexpected(BombardErrorReason::InvalidSectorFormat);
    }
    target_coords = *coords_opt;
    if (!p.is_valid(target_coords)) {
      return std::unexpected(BombardErrorReason::IllegalSector);
    }
  } else {
    target_coords = em.peek_sectormap(from.storbits(), from.pnumorbits())
                        ->get_random()
                        .coords();
  }
  if (from.is_landed() && !p.is_adjacent(from.land_coords(), target_coords)) {
    return std::unexpected(BombardErrorReason::NotAdjacentToSector);
  }
  return target_coords;
}

void resolve_bombard_retaliation(EntityManager& em, Ship& from, Planet& p,
                                 BombardShipResult& res) {
  if (!res.bombard->sectors_destroyed || from.type() == ShipType::OTYPE_AFV) {
    return;
  }
  if (!p.is_enslaved()) {
    const auto* star = em.peek_star(from.storbits());
    for (player_t i : res.bombard->nuked_players) {
      const auto* alien = em.peek_race(i);
      const weapon_power_t retal_strength = std::min(
          static_cast<weapon_power_t>(p.info(i).destruct), p.info(i).guns);
      p.info(i).destruct -= retal_strength;
      if (auto shot = shoot_planet_to_ship(em, *alien, from, retal_strength)) {
        res.planet_retaliation.push_back({
            .defender_player = i,
            .defender_governor = star->governor(i),
            .shot = std::move(*shot),
        });
      }
    }
  }
  for (auto ship_handle :
       ShipList::on_planet(em, from.storbits(), from.pnumorbits())) {
    if (!from.alive()) break;
    Ship& ship = *ship_handle;
    if (!ship.protect().planet || ship.number() == from.number() ||
        !ship.active()) {
      continue;
    }
    if (auto event = execute_escort_shot(em, ship, from)) {
      res.protector_retaliation.push_back(std::move(*event));
    }
  }
}

std::optional<DefendPlanetRetaliationEvent>
execute_planet_retaliation_shot(EntityManager& em, Ship& ship, Planet& p,
                                SectorMap& smap, Coordinates sector_coords,
                                weapon_power_t initial_strength) {
  auto [strength, overload] = check_overload(em, ship, 0, initial_strength);
  if (overload) {
    return DefendPlanetRetaliationEvent{
        .ship_owner = ship.owner(),
        .ship_governor = ship.governor(),
        .overload = std::move(overload),
    };
  }
  auto bombard = shoot_ship_to_planet(em, ship, p, strength, sector_coords,
                                      smap, false, guntype_t::NONE);
  if (!bombard) {
    return std::nullopt;
  }
  ship.consume_weapon_resources(strength);
  return DefendPlanetRetaliationEvent{
      .ship_owner = ship.owner(),
      .ship_governor = ship.governor(),
      .bombard = std::move(bombard),
  };
}

std::expected<weapon_power_t, DefendError>
validate_defend_target(EntityManager& em, starnum_t snum, planetnum_t pnum,
                       shipnum_t toship) {
  const Ship* to = nullptr;
  try {
    to = em.peek_ship(toship);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::ShipNotFound});
  }
  if (!to->alive()) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::ShipAlreadyDestroyed});
  }
  if (to->whatorbits() != ScopeLevel::LEVEL_PLAN) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::ShipNotInPlanetOrbit});
  }
  if (to->storbits() != snum || to->pnumorbits() != pnum) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::TargetNotOrbitingThisPlanet});
  }
  if (to->is_landed()) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::CannotFireOnLandedShip});
  }
  return to->check_retal_strength();
}

std::expected<std::pair<Coordinates, weapon_power_t>, DefendError>
validate_defend_sector_and_strength(
    EntityManager& em, starnum_t snum, planetnum_t pnum, player_t player,
    std::string_view sector_arg,
    std::optional<weapon_power_t> requested_strength) {
  auto coords_opt = Coordinates::parse(sector_arg);
  if (!coords_opt) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::BadSectorFormat});
  }
  const Coordinates sector_coords = *coords_opt;

  const auto& p_peek = *em.peek_planet(snum, pnum);
  if (!p_peek.info(player).numsectsowned) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::NoSectorsOccupied});
  }
  if (p_peek.is_enslaved_to_foreign(player)) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::PlanetEnslaved});
  }
  if (!p_peek.is_valid(sector_coords)) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::IllegalSector});
  }
  if (em.peek_sectormap(snum, pnum)->get(sector_coords).get_owner() != player) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::SectorNotOwned});
  }

  weapon_power_t strength =
      requested_strength ? std::max<weapon_power_t>(0, *requested_strength)
                         : p_peek.info(player).guns;
  strength =
      std::min(strength, static_cast<weapon_power_t>(std::max<resource_t>(
                             0, p_peek.info(player).destruct)));
  strength = std::min(strength, p_peek.info(player).guns);
  if (strength <= 0) {
    return std::unexpected(DefendError{
        .reason = DefendErrorReason::NoAttack,
        .guns = p_peek.info(player).guns,
        .destruct = p_peek.info(player).destruct,
    });
  }
  return std::pair{sector_coords, strength};
}

void resolve_defend_retaliation(EntityManager& em, starnum_t snum,
                                planetnum_t pnum, Ship& target_ship, Planet& p,
                                SectorMap& smap, Coordinates sector_coords,
                                weapon_power_t retal, DefendPlanetResult& res) {
  if (res.planet_shot.damage <= 0) {
    return;
  }
  if (retal > 0 && target_ship.protect().retaliate) {
    if (auto ev = execute_planet_retaliation_shot(em, target_ship, p, smap,
                                                  sector_coords, retal)) {
      res.ship_retaliation.push_back(std::move(*ev));
    }
  }
  for (auto ship_handle : ShipList::on_planet(em, snum, pnum)) {
    Ship& ship = *ship_handle;
    if (!is_eligible_escort(ship, target_ship.number(), 0)) {
      continue;
    }
    if (auto ev = execute_planet_retaliation_shot(
            em, ship, p, smap, sector_coords, ship.check_retal_strength())) {
      res.ship_retaliation.push_back(std::move(*ev));
    }
  }
}

}  // namespace

std::optional<ShipCombatExchange>
execute_defensive_fire(EntityManager& em, Ship& attacker, Ship& defender) {
  if (!defender.alive() || !defender.active()) {
    return std::nullopt;
  }

  const weapon_power_t initial_strength = defender.check_retal_strength();
  if (initial_strength <= 0) {
    return std::nullopt;
  }

  ShipCombatExchange exchange{
      .shooter_owner = defender.owner(),
      .shooter_governor = defender.governor(),
      .target_owner = attacker.owner(),
      .target_governor = attacker.governor(),
      .star_id = defender.storbits(),
  };

  auto [strength, overload] = check_overload(em, defender, 0, initial_strength);
  if (overload) {
    exchange.primary_overload = overload;
    exchange.primary_overload_fizzled = true;
    return exchange;
  }

  const auto retal = attacker.check_retal_strength();
  exchange.primary_shot =
      shoot_ship_to_ship(em, defender, attacker, strength, 0);
  if (!exchange.primary_shot) {
    return std::nullopt;
  }

  defender.consume_weapon_resources(strength);
  const damage_t damage = exchange.primary_shot->damage;
  resolve_target_self_retaliation(em, attacker, defender, retal, damage,
                                  exchange);
  resolve_escort_retaliation(em, attacker, defender, damage, exchange);
  return exchange;
}

std::expected<FireShipResult, FireError>
fire_single_ship(EntityManager& em, Ship& from, shipnum_t target_id,
                 std::optional<weapon_power_t> requested_strength,
                 player_t player, bool god) {
  if (!from.active()) {
    return std::unexpected(FireError{
        .reason = FireErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", from),
    });
  }
  if (target_id == from.number()) {
    return std::unexpected(
        FireError{.reason = FireErrorReason::CannotFireAtSelf});
  }

  const Ship* to = nullptr;
  try {
    to = em.peek_ship(target_id);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(FireError{
        .reason = FireErrorReason::TargetNotFound,
        .abort_loop = true,
    });
  }

  if (auto geom_err = validate_surface_combat_geometry(em, from, *to)) {
    return std::unexpected(*geom_err);
  }

  const weapon_power_t maxstrength = from.check_retal_strength();
  weapon_power_t strength = requested_strength.value_or(maxstrength);
  FireShipResult result{};
  if (strength > maxstrength) {
    strength = maxstrength;
    result.clamped_strength = strength;
    result.clamped_is_laser = from.is_laser_on();
  }

  if (auto attack_err = execute_ship_attack(em, from, target_id, strength,
                                            false, player, god, result)) {
    return std::unexpected(FireError{
        .reason = *attack_err,
        .clamped_strength = result.clamped_strength,
        .clamped_is_laser = result.clamped_is_laser,
    });
  }

  return result;
}

std::expected<FireShipResult, FireError>
cew_single_ship(EntityManager& em, Ship& from, shipnum_t target_id,
                player_t player, bool god) {
  if (!from.active()) {
    return std::unexpected(FireError{
        .reason = FireErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", from),
    });
  }
  if (target_id == from.number()) {
    return std::unexpected(
        FireError{.reason = FireErrorReason::CannotFireAtSelf});
  }

  const Ship* to = nullptr;
  try {
    to = em.peek_ship(target_id);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(FireError{
        .reason = FireErrorReason::TargetNotFound,
        .abort_loop = true,
    });
  }

  if (auto geom_err = validate_surface_combat_geometry(em, from, *to)) {
    return std::unexpected(*geom_err);
  }
  if (auto cew_err = validate_cew_fire(from, *to)) {
    return std::unexpected(*cew_err);
  }

  FireShipResult result{
      .cew_strength = from.cew(),
  };
  const weapon_power_t initial_strength = from.cew() / 2;
  if (auto attack_err = execute_ship_attack(
          em, from, target_id, initial_strength, true, player, god, result)) {
    return std::unexpected(FireError{
        .reason = *attack_err,
        .cew_strength = from.cew(),
    });
  }

  return result;
}

std::expected<BombardShipResult, BombardError>
bombard_single_ship(EntityManager& em, Ship& from,
                    std::optional<std::string_view> coords_arg,
                    std::optional<weapon_power_t> requested_strength,
                    player_t player, bool god) {
  if (!from.active()) {
    return std::unexpected(BombardError{
        .reason = BombardErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", from),
    });
  }
  if (from.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    return std::unexpected(
        BombardError{.reason = BombardErrorReason::NotInPlanetOrbit});
  }
  if (from.type() == ShipType::OTYPE_AFV && !from.is_landed()) {
    return std::unexpected(
        BombardError{.reason = BombardErrorReason::AfvNotLanded});
  }
  if (requested_strength && *requested_strength < 0) {
    return std::unexpected(
        BombardError{.reason = BombardErrorReason::NoAttack});
  }

  const weapon_power_t maxstrength = from.check_retal_strength();
  weapon_power_t strength = requested_strength.value_or(maxstrength);
  BombardShipResult res{.star_id = from.storbits()};
  if (strength > maxstrength) {
    strength = maxstrength;
    res.clamped_strength = strength;
    res.clamped_is_laser = from.is_laser_on();
  }

  std::optional<BombardErrorReason> err;
  em.mutate_planet(from.storbits(), from.pnumorbits(), [&](Planet& p) {
    auto coords = resolve_bombard_coords(em, from, p, coords_arg, player);
    if (!coords) {
      err = coords.error();
      return;
    }
    if (strength <= 0) {
      err = BombardErrorReason::NoAttack;
      return;
    }
    if (check_fire_ap(em, from, player, god)) {
      err = BombardErrorReason::InsufficientAp;
      return;
    }
    auto [effective_strength, overload] = check_overload(em, from, 0, strength);
    if (overload) {
      res.bomber_overload = std::move(overload);
      deduct_fire_ap(em, from, player, god);
      return;
    }
    em.mutate_sectormap(
        from.storbits(), from.pnumorbits(), [&](SectorMap& smap) {
          res.bombard =
              shoot_ship_to_planet(em, from, p, effective_strength, *coords,
                                   smap, false, guntype_t::NONE);
        });
    if (!res.bombard) {
      err = BombardErrorReason::IllegalAttack;
      return;
    }
    deduct_fire_ap(em, from, player, god);
    from.consume_weapon_resources(effective_strength);
    resolve_bombard_retaliation(em, from, p, res);
  });

  if (err) {
    return std::unexpected(BombardError{
        .reason = *err,
        .clamped_strength = res.clamped_strength,
        .clamped_is_laser = res.clamped_is_laser,
    });
  }
  return res;
}

std::expected<DefendPlanetResult, DefendError>
defend_planet_against_ship(EntityManager& em, starnum_t snum, planetnum_t pnum,
                           player_t player, shipnum_t toship,
                           std::string_view sector_arg,
                           std::optional<weapon_power_t> requested_strength) {
  auto retal = validate_defend_target(em, snum, pnum, toship);
  if (!retal) {
    return std::unexpected(retal.error());
  }
  auto sector_and_strength = validate_defend_sector_and_strength(
      em, snum, pnum, player, sector_arg, requested_strength);
  if (!sector_and_strength) {
    return std::unexpected(sector_and_strength.error());
  }
  const auto [sector_coords, strength] = *sector_and_strength;

  const auto& race = *em.peek_race(player);
  std::optional<DefendPlanetResult> res;
  em.mutate_ship(toship, [&](Ship& target_ship) {
    em.mutate_planet(snum, pnum, [&](Planet& p) {
      em.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
        auto p2s_opt = shoot_planet_to_ship(em, race, target_ship, strength);
        if (!p2s_opt) {
          return;
        }
        p.info(player).destruct -= strength;
        res = DefendPlanetResult{
            .star_id = target_ship.storbits(),
            .target_owner = target_ship.owner(),
            .target_governor = target_ship.governor(),
            .planet_shot = std::move(*p2s_opt),
        };
        resolve_defend_retaliation(em, snum, pnum, target_ship, p, smap,
                                   sector_coords, *retal, *res);
      });
    });
  });

  if (!res) {
    return std::unexpected(
        DefendError{.reason = DefendErrorReason::TargetOutOfRange});
  }
  return *res;
}
