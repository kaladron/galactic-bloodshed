// SPDX-License-Identifier: Apache-2.0

/// \file construction.cc
/// \brief Ship construction validation, factory spawning, and shipping cost
/// mechanics implementation.

module;

import std;

module gb.mechanics;

/**
 * @brief Determines if a ship can be built on a specific sector of a planet.
 *
 * This function checks various conditions to determine whether a ship of the
 * specified type can be built on the given sector of a planet. If the sector
 * is not suitable for building, it returns an error message explaining the
 * reason. Otherwise, it returns success.
 *
 * @param what The type of ship to be built, represented as an integer.
 * @param race The race attempting to build the ship.
 * @param planet The planet on which the sector is located.
 * @param sector The sector where the ship is to be built.
 * @param c The x and y coordinates of the sector.
 * @return std::expected<void, std::string> Success or an error message string.
 */
std::expected<void, std::string>
validate_sector_build_site(EntityManager& entity_manager, const ShipType what,
                           const Race& race, const Planet& planet,
                           const Sector& sector, const Coordinates& c) {
  if (!sector.get_popn()) {
    return std::unexpected("You have no more civs in the sector!\n");
  }
  if (sector.is_wasted()) {
    return std::unexpected("You can't build on wasted sectors.\n");
  }
  if (sector.get_owner() != race.Playernum && !race.God) {
    return std::unexpected("You don't own that sector.\n");
  }
  if (what == ShipType::OTYPE_QUARRY) {
    for (const Ship& s : ShipList::readonly_on_planet(
             entity_manager, planet.star_id(), planet.planet_order())) {
      if (s.type() == ShipType::OTYPE_QUARRY && s.land_coords() == c) {
        return std::unexpected("There already is a quarry here.\n");
      }
    }
  }
  return {};
}

std::expected<void, std::string>
can_build_on_sector(EntityManager& entity_manager, const ShipType what,
                    const Race& race, const Planet& planet,
                    const Sector& sector, const Coordinates& c) {
  if (const auto site_ok = validate_sector_build_site(entity_manager, what,
                                                      race, planet, sector, c);
      !site_ok) {
    return site_ok;
  }
  if (!ship_template(what).can_build_on_planet() && !race.God) {
    return std::unexpected(std::format(
        "This ship type cannot be built on a planet.\nUse 'build ? {}' to find "
        "out where it can be built.\n",
        ship_template(what).letter));
  }
  return {};
}

// Used for optional parameters.  If the element requested exists, use
// it.  If the number is negative or non-numeric, return zero instead.
int getcount(const command_t& argv, const std::size_t elem) {
  if (argv.size() <= elem) {
    return 1;
  }
  auto res = parse_int<int>(argv[elem]);
  if (!res) {
    return 0;
  }
  return std::max(*res, 0);
}

std::expected<void, PlanetBuildError>
can_build_at_planet(const player_t playernum, const governor_t governor,
                    const Star& star, const Planet& planet) {
  if (planet.is_enslaved_to_foreign(playernum)) {
    return std::unexpected(PlanetBuildError{
        .reason = PlanetBuildErrorReason::EnslavedByForeignPlayer,
        .enslaving_player = *planet.slaved_to(),
    });
  }
  if (!star.control(playernum, governor)) {
    return std::unexpected(PlanetBuildError{
        .reason = PlanetBuildErrorReason::NotAuthorizedInSystem,
    });
  }
  return {};
}

std::optional<ShipType> get_build_type(const char shipc) {
  for (const auto& tmpl : ship_templates) {
    if (tmpl.letter == shipc) return tmpl.type;
  }
  return std::nullopt;
}

std::expected<void, std::string> can_build_this(const ShipType what,
                                                const Race& race) {
  if (what == ShipType::STYPE_POD && !race.pods) {
    return std::unexpected("Only Metamorphic races can build Spore Pods.\n");
  }
  const auto& tmpl = ship_template(what);
  if (!tmpl.is_programmed) {
    return std::unexpected("This ship type has not been programmed.\n");
  }
  if (tmpl.is_god_only && !race.God) {
    return std::unexpected("Only Gods can build this type of ship.\n");
  }
  if (what == ShipType::OTYPE_VN && !race.discoveries.vn) {
    return std::unexpected("You have not discovered VN technology.\n");
  }
  if (what == ShipType::OTYPE_TRANSDEV && !race.discoveries.avpm) {
    return std::unexpected("You have not discovered AVPM technology.\n");
  }
  if (tmpl.base_tech > race.tech && !race.God) {
    std::string error = std::format(
        "You are not advanced enough to build this ship.\n{:.1f} engineering "
        "technology needed. You have {:.1f}.\n",
        tmpl.base_tech, race.tech);
    return std::unexpected(error);
  }
  return {};
}

std::expected<void, std::string>
can_build_on_ship(ShipType what, const Race& race, const Ship& builder) {
  if (!ship_template(what).can_be_built_by(builder.get_template()) &&
      !race.God) {
    std::string error = std::format(
        "This ship type cannot be built by a {}.\nUse 'build ? {}' to find out "
        "where it can be built.\n",
        builder.get_template().name, ship_template(what).letter);
    return std::unexpected(error);
  }
  return {};
}

std::expected<ShipBuildLocation, ShipBuildError>
build_at_ship(const player_t playernum, const governor_t governor,
              const bool god, const Ship& builder, EntityManager* em) {
  if (const auto cmd_ok =
          validate_commandable(builder, playernum, governor, god);
      !cmd_ok) {
    switch (cmd_ok.error()) {
      case CommandableError::ShipDead:
        return std::unexpected(ShipBuildError::ShipDead);
      case CommandableError::ShipIrradiated:
        return std::unexpected(ShipBuildError::ShipIrradiated);
      case CommandableError::NotOwner:
        return std::unexpected(ShipBuildError::NotOwner);
      case CommandableError::NotAuthorizedGovernor:
        return std::unexpected(ShipBuildError::NotAuthorizedGovernor);
    }
  }
  if (!builder.can_construct_ships()) {
    return std::unexpected(ShipBuildError::CannotConstructShips);
  }
  if (!builder.popn()) {
    return std::unexpected(ShipBuildError::NoCrew);
  }
  const bool berthed_factory = builder.type() == ShipType::OTYPE_FACTORY &&
                               builder.whatorbits() == ScopeLevel::LEVEL_SHIP;
  if (builder.is_docked() && !berthed_factory) {
    return std::unexpected(ShipBuildError::ShipDocked);
  }
  if (builder.damage()) {
    return std::unexpected(ShipBuildError::ShipDamaged);
  }
  if (builder.type() == ShipType::OTYPE_FACTORY) {
    if (!builder.on()) {
      return std::unexpected(ShipBuildError::FactoryNotOnline);
    }
    if (builder.whatorbits() == ScopeLevel::LEVEL_SHIP && em) {
      const auto& carrier = *em->peek_ship(*builder.destshipno());
      if (carrier.type() == ShipType::STYPE_HABITAT) {
        return ShipBuildLocation{
            .level = ScopeLevel::LEVEL_SHIP,
            .snum = carrier.storbits(),
            .pnum = carrier.pnumorbits(),
        };
      }
    }
    if (!builder.is_landed()) {
      return std::unexpected(ShipBuildError::FactoryNotLanded);
    }
  }
  return ShipBuildLocation{
      .level = builder.whatorbits(),
      .snum = builder.storbits(),
      .pnum = builder.pnumorbits(),
  };
}

std::pair<population_t, fuel_t> autoload_at_planet(player_t Playernum,
                                                   const Ship& s,
                                                   Planet& planet,
                                                   Sector& sector) {
  const population_t crew = std::min(s.max_crew_capacity(), sector.get_popn());
  const fuel_t fuel =
      std::min(static_cast<fuel_t>(s.max_fuel_capacity()),
               static_cast<fuel_t>(planet.info(Playernum).fuel));
  planet.adjust_sector_population(sector, Playernum, -crew, 0);
  planet.info(Playernum).fuel -= static_cast<resource_t>(fuel);
  return {crew, fuel};
}

std::pair<population_t, fuel_t> autoload_at_ship(const Ship& s, Ship& b,
                                                 double race_mass) {
  const population_t crew = std::min(s.max_crew_capacity(), b.popn());
  const fuel_t fuel =
      std::min(static_cast<fuel_t>(s.max_fuel_capacity()), b.fuel());
  b.remove_popn(crew, race_mass);
  b.consume_fuel(fuel);
  return {crew, fuel};
}

InitializedShipReport initialize_new_ship(const Race& race,
                                          const governor_t governor,
                                          Ship& newship, const fuel_t load_fuel,
                                          const population_t load_crew) {
  newship.initialize_constructed_state(race, governor, load_fuel, load_crew);
  return InitializedShipReport{
      .ship_type = newship.type(),
      .tele_range = newship.tele_range(),
      .damage = newship.damage(),
      .can_repair = newship.can_repair(),
      .has_crew_capacity = newship.max_crew_capacity() > 0,
      .loaded_crew = load_crew,
      .loaded_fuel = load_fuel,
  };
}

CreatedShipSummary create_ship_by_planet(EntityManager& entity_manager,
                                         player_t Playernum,
                                         governor_t Governor, const Race& race,
                                         Ship& newship, Planet& planet,
                                         starnum_t snum, planetnum_t pnum,
                                         Coordinates land_coords) {
  newship.tech() = race.tech;
  const auto& star = *entity_manager.peek_star(snum);
  newship.set_coordinates(planet.absolute_coordinates(star));
  if (newship.shipclass().empty()) {
    newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                            (newship.type() == ShipType::OTYPE_PLOW))
                               ? "5"
                               : "Standard");
  }
  newship.land_on_planet(snum, pnum, land_coords);
  planet.info(Playernum).resource -= newship.build_cost();

  newship.number() = entity_manager.next_available_ship_number();
  newship.owner() = Playernum;
  newship.governor() = Governor;

  std::optional<Percentage> prev_tox{std::nullopt};
  std::optional<Percentage> new_tox{std::nullopt};
  if (auto* waste_ship = newship.as<ToxicWasteShip>()) {
    prev_tox = planet.toxic();
    const int toxic_amount = std::min(TOXMAX, static_cast<int>(planet.toxic()));
    waste_ship->set_toxic_level(toxic_amount);
    planet.toxic() -= toxic_amount;
    new_tox = planet.toxic();
  }

  return CreatedShipSummary{
      .ship_display = std::format("{}", newship),
      .build_cost = newship.build_cost(),
      .tech = newship.tech(),
      .landed_sector = newship.land_coords(),
      .previous_toxicity = prev_tox,
      .updated_toxicity = new_tox,
  };
}

CreatedShipSummary create_ship_by_ship(EntityManager& entity_manager,
                                       player_t Playernum, governor_t Governor,
                                       const Race& race, bool outside,
                                       Ship& newship, Ship& builder) {
  newship.number() = entity_manager.next_available_ship_number();
  newship.owner() = Playernum;
  newship.governor() = Governor;
  if (outside) {
    newship.launch_from_carrier_to_orbit(builder);
  } else {
    const auto orbit = entity_manager.effective_ship_orbit(builder);
    newship.dock_into_carrier(builder);
    newship.sync_carrier_orbit(orbit.level, orbit.snum, orbit.pnum,
                               builder.coordinates(), builder.land_coords());
  }
  newship.tech() = race.tech;
  newship.set_coordinates(builder.coordinates());
  newship.set_land_coords(builder.land_coords());
  if (newship.shipclass().empty()) {
    newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                            (newship.type() == ShipType::OTYPE_PLOW))
                               ? "5"
                               : "Standard");
  }
  builder.consume_resource(newship.build_cost());

  return CreatedShipSummary{
      .ship_display = std::format("{}", newship),
      .build_cost = newship.build_cost(),
      .tech = newship.tech(),
      .landed_sector = std::nullopt,
      .previous_toxicity = std::nullopt,
      .updated_toxicity = std::nullopt,
  };
}

std::unique_ptr<Ship> getship(ShipType i, const Race& r) {
  auto ship = ShipFactory::create_from_template(i, r.Playernum);
  const auto& tmpl = ship_template(i);
  ship->mount() = r.God && tmpl.can_mount;
  ship->hyper_drive() = {.has = r.God && tmpl.can_hyperjump};
  ship->laser() = r.God && tmpl.can_mount_laser;
  ship->recalculate_hull_stats(r.God);
  return ship;
}

std::unique_ptr<Ship> getfactship(const FactoryShip& b) {
  return b.instantiate_blueprint();
}

resource_t Shipcost(ShipType i, const Race& r) {
  auto s = getship(i, r);
  return cost(*s);
}

std::tuple<money_t, double> shipping_cost(EntityManager& em, const starnum_t to,
                                          const starnum_t from,
                                          const money_t value) {
  const auto* star_to = em.peek_star(to);
  const auto* star_from = em.peek_star(from);

  const double dist =
      star_to->coordinates().distance_to(star_from->coordinates());
  if (value <= 0 || dist <= 0.0) {
    return {0, dist};
  }

  const double quantized_dist = std::floor(dist / 10000.0) * 10000.0;
  const double factor = -std::expm1(-quantized_dist / MERCHANT_LENGTH);
  const double raw_cost = std::round(factor * static_cast<double>(value));
  const money_t fcost =
      raw_cost >= static_cast<double>(std::numeric_limits<money_t>::max())
          ? std::numeric_limits<money_t>::max()
          : static_cast<money_t>(raw_cost);
  return {fcost, dist};
}

std::optional<std::pair<starnum_t, planetnum_t>>
resolve_ship_planet_orbit(EntityManager& em, const Ship& ship) {
  const auto orbit = em.effective_ship_orbit(ship);
  if (orbit.level == ScopeLevel::LEVEL_PLAN) {
    return std::pair{orbit.snum, orbit.pnum};
  }
  return std::nullopt;
}

std::expected<ScrapShipResult, ScrapError>
scrap_single_ship(EntityManager& em, Ship& s, const Race& race) {
  if (s.max_crew_capacity() > 0 && s.popn() == 0) {
    return std::unexpected(ScrapError::NoCrew);
  }

  if (s.is_docked() && s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
    const Ship& s2 = *em.peek_ship(*s.destshipno());
    if (!s2.docked() || s2.destshipno() != s.number()) {
      return std::unexpected(ScrapError::OtherShipNotDocked);
    }
  }

  const auto orbit = em.effective_ship_orbit(s);
  const bool in_deep_space = (orbit.level == ScopeLevel::LEVEL_UNIV);
  if (in_deep_space) {
    if (!race.God) {
      const auto* univ = em.peek_universe();
      if (univ->get_AP(race.Playernum) < 1) {
        return std::unexpected(ScrapError::InsufficientUniverseAp);
      }
      em.mutate_universe([&](Universe& u) { u.deduct_AP(race.Playernum, 1); });
    }
  } else {
    const auto* star = em.peek_star(orbit.snum);
    if (!star) {
      return std::unexpected(ScrapError::StarNotFound);
    }
    if (!race.God) {
      if (star->AP(race.Playernum) < 1) {
        return std::unexpected(ScrapError::InsufficientStarAp);
      }
      em.mutate_star(orbit.snum, [&](Star& st) { st.AP(race.Playernum) -= 1; });
    }
  }

  ScrapShipResult result{
      .ship_display = std::format("{}", s),
      .was_landed = s.is_landed(),
      .reclaimed = s.docked(),
  };

  if (const auto* tox = s.as<ToxicWasteShip>()) {
    if (resolve_ship_planet_orbit(em, s).has_value()) {
      result.toxin_released = tox->toxic_level();
    }
  }

  const bool is_landed_on_planet = s.is_landed();
  player_t sect_owner = 0;
  if (is_landed_on_planet) {
    sect_owner = em.peek_sectormap(s.storbits(), s.pnumorbits())
                     ->get(s.land_coords())
                     .get_owner();
  }
  const bool foreign_sector =
      (is_landed_on_planet && sect_owner != 0 && sect_owner != race.Playernum);

  resource_t scrapval =
      s.effective_cost() / SCRAP_RECOVERY_DIVISOR + s.resource();
  fuel_t fuelval = s.fuel();
  resource_t destval = s.destruct();
  population_t troopval = s.troops();
  population_t crewval = s.popn();
  int xtalval = s.crystals() + s.mounted();

  if (s.docked()) {
    result.original_cost = s.effective_cost();
    result.has_resource_stockpile = (s.resource() > 0);
    result.initial_scrap_value = scrapval;
    result.initial_fuel = fuelval;
    result.initial_destruct = destval;
    result.initial_popn = crewval;
    result.initial_troops = troopval;
    result.initial_crystals = xtalval;

    if (foreign_sector) {
      if (crewval + troopval > 0) {
        result.foreign_sector_blocks_crew = true;
        crewval = 0;
        troopval = 0;
      }
      if (xtalval > 0) {
        result.foreign_sector_blocks_crystals = true;
        xtalval = 0;
      }
    }

    if (s.is_docked()) {
      const Ship& s2 = *em.peek_ship(*s.destshipno());
      if (!s2.can_strap_cargo_to_hull() &&
          s2.resource() + scrapval > s2.max_resource_capacity()) {
        scrapval =
            std::max<resource_t>(0, s2.max_resource_capacity() - s2.resource());
        result.resource_room_limit = scrapval;
      }
      if (s2.fuel() + fuelval > s2.max_fuel_capacity()) {
        fuelval = std::max(0.0, s2.max_fuel_capacity() - s2.fuel());
        result.fuel_room_limit = fuelval;
      }
      if (s2.destruct() + destval > s2.max_destruct_capacity()) {
        destval =
            std::max<resource_t>(0, s2.max_destruct_capacity() - s2.destruct());
        result.destruct_room_limit = destval;
      }
      if (crewval + troopval > 0) {
        const population_t avail_berths = s2.available_crew_capacity();
        if (troopval > avail_berths) {
          troopval = avail_berths;
          result.troops_room_limit = troopval;
        }
        const population_t remaining_berths = avail_berths - troopval;
        if (crewval > remaining_berths) {
          crewval = remaining_berths;
          result.crew_room_limit = crewval;
        }
      }
      if (s2.crystals() + xtalval > s2.max_crystals_capacity()) {
        xtalval =
            s2.max_crystals_capacity() > s2.crystals()
                ? static_cast<int>(s2.max_crystals_capacity() - s2.crystals())
                : 0;
        result.crystals_room_limit = xtalval;
      }
    }
    result.recovered_crystals = xtalval;
  }

  if (s.is_docked()) {
    const shipnum_t dest_ship = *s.destshipno();
    double s2_gain = 0.0;
    em.mutate_ship(dest_ship, [&](Ship& s2) {
      const double old_mass = s2.mass();
      s2.add_crystals(xtalval);
      s2.add_fuel(fuelval);
      s2.add_destruct(destval);
      s2.add_resource(scrapval);
      s2.add_troops(troopval, race.mass);
      s2.add_popn(crewval, race.mass);
      s2_gain = s2.mass() - old_mass;
    });
    em.propagate_carried_mass_delta(*em.peek_ship(dest_ship), s2_gain);
  }

  kill_ship(em, race.Playernum, s);

  if (is_landed_on_planet) {
    em.mutate_sectormap(s.storbits(), s.pnumorbits(), [&](SectorMap& smap) {
      em.mutate_planet(s.storbits(), s.pnumorbits(), [&](Planet& planet) {
        auto& sector = smap.get(s.land_coords());
        const bool was_unowned = !sector.is_owned();
        planet.adjust_sector_population(sector, race.Playernum, crewval,
                                        troopval);
        if (was_unowned && sector.is_owned()) {
          result.colonized_sector = s.land_coords();
        }
        planet.info(race.Playernum).resource += scrapval;
        planet.info(race.Playernum).destruct += destval;
        planet.info(race.Playernum).fuel += static_cast<int>(fuelval);
        planet.info(race.Playernum).crystals += xtalval;
      });
    });
  }

  return result;
}

namespace {

constexpr double MAX_FACTORY_SHIP_COST = 65535.0;
constexpr armor_t MAX_FACTORY_ARMOR = 100;
constexpr int MAX_FACTORY_CAPACITY = 10000;
constexpr speed_t MIN_FACTORY_SPEED = 1;
constexpr speed_t MAX_FACTORY_SPEED = 9;

std::optional<int> parse_non_negative_int(std::string_view arg) {
  auto res = parse_int<int>(arg);
  if (!res || *res < 0) {
    return std::nullopt;
  }
  return *res;
}

std::expected<void, FactoryModifyError>
modify_battery(FactoryBlueprint& bp, const command_t& argv, bool is_primary) {
  if (argv.size() < 4) {
    return std::unexpected(FactoryModifyError::InvalidGunCharacteristic);
  }
  auto& bat = is_primary ? bp.primary_battery : bp.secondary_battery;
  if (argv[2] == "strength") {
    auto strength = parse_non_negative_int(argv[3]);
    if (!strength) {
      return std::unexpected(FactoryModifyError::RidiculousSetting);
    }
    bat.set(*strength, bat.caliber);
    return {};
  }
  if (argv[2] == "caliber") {
    auto new_caliber = parse_caliber_name(argv[3]);
    if (!new_caliber) {
      return std::unexpected(FactoryModifyError::InvalidCaliber);
    }
    const auto max_cal = is_primary ? shipdata_primary(bp.build_type)
                                    : shipdata_secondary(bp.build_type);
    bat.set(bat.count, std::min(max_cal, *new_caliber));
    return {};
  }
  return std::unexpected(FactoryModifyError::InvalidGunCharacteristic);
}

std::expected<void, FactoryModifyError>
modify_cew(FactoryBlueprint& bp, const Race& race, const command_t& argv) {
  if (!race.discoveries.cew) {
    return std::unexpected(FactoryModifyError::NoCewDiscovery);
  }
  if (argv.size() < 4) {
    return std::unexpected(FactoryModifyError::InvalidCewOption);
  }
  auto value = parse_non_negative_int(argv[3]);
  if (!value) {
    return std::unexpected(FactoryModifyError::RidiculousSetting);
  }
  if (argv[2] == "strength") {
    bp.cew = *value;
    return {};
  }
  if (argv[2] == "range") {
    bp.cew_range = *value;
    return {};
  }
  return std::unexpected(FactoryModifyError::InvalidCewOption);
}

enum class ShipAttr {
  Primary,
  Secondary,
  Cew,
  Mount,
  Hyperdrive,
  Laser,
  Armor,
  Crew,
  Cargo,
  Hangar,
  Fuel,
  Destruct,
  Speed,
};

std::optional<ShipAttr> parse_ship_attribute(std::string_view name) {
  static constexpr std::array<std::pair<std::string_view, ShipAttr>, 14> table{{
      {"primary", ShipAttr::Primary},
      {"secondary", ShipAttr::Secondary},
      {"cew", ShipAttr::Cew},
      {"mount", ShipAttr::Mount},
      {"hyperdrive", ShipAttr::Hyperdrive},
      {"laser", ShipAttr::Laser},
      {"armor", ShipAttr::Armor},
      {"crew", ShipAttr::Crew},
      {"cargo", ShipAttr::Cargo},
      {"resource", ShipAttr::Cargo},
      {"hangar", ShipAttr::Hangar},
      {"fuel", ShipAttr::Fuel},
      {"destruct", ShipAttr::Destruct},
      {"speed", ShipAttr::Speed},
  }};
  for (const auto& [key, val] : table) {
    if (name == key) return val;
  }
  return std::nullopt;
}

std::expected<void, FactoryModifyError>
modify_simple_attribute(FactoryBlueprint& bp, const Race& race,
                        const ShipTemplate& btmpl, ShipAttr attr,
                        const command_t& argv) {
  switch (attr) {
    case ShipAttr::Mount:
      if (!btmpl.can_mount) break;
      if (!race.discoveries.crystal) break;
      bp.mount = !bp.mount;
      return {};
    case ShipAttr::Hyperdrive:
      if (!btmpl.can_hyperjump) break;
      if (!race.discoveries.hyperdrive) break;
      bp.has_hyperdrive = !bp.has_hyperdrive;
      return {};
    case ShipAttr::Laser:
      if (!btmpl.can_mount_laser) break;
      if (!race.discoveries.laser) {
        return std::unexpected(FactoryModifyError::NoLaserDiscovery);
      }
      bp.laser = !bp.laser;
      return {};
    default:
      break;
  }

  int value = 0;
  if (argv.size() >= 3) {
    auto parsed = parse_non_negative_int(argv[2]);
    if (!parsed) {
      return std::unexpected(FactoryModifyError::RidiculousSetting);
    }
    value = *parsed;
  }

  switch (attr) {
    case ShipAttr::Armor:
      bp.armor = std::min<armor_t>(value, MAX_FACTORY_ARMOR);
      return {};
    case ShipAttr::Crew:
      if (!btmpl.max_crew) break;
      bp.max_crew = std::min<population_t>(value, MAX_FACTORY_CAPACITY);
      return {};
    case ShipAttr::Cargo:
      if (!btmpl.max_resource) break;
      bp.max_resource = std::min<resource_t>(value, MAX_FACTORY_CAPACITY);
      return {};
    case ShipAttr::Hangar:
      if (!btmpl.max_hangar) break;
      bp.max_hanger = std::min<hangar_t>(value, MAX_FACTORY_CAPACITY);
      return {};
    case ShipAttr::Fuel:
      if (!btmpl.max_fuel) break;
      bp.max_fuel = std::min<fuel_t>(value, MAX_FACTORY_CAPACITY);
      return {};
    case ShipAttr::Destruct:
      if (!btmpl.max_destruct) break;
      bp.max_destruct = std::min<resource_t>(value, MAX_FACTORY_CAPACITY);
      return {};
    case ShipAttr::Speed:
      if (!btmpl.base_speed) break;
      bp.max_speed =
          std::clamp<speed_t>(value, MIN_FACTORY_SPEED, MAX_FACTORY_SPEED);
      return {};
    default:
      break;
  }

  return std::unexpected(FactoryModifyError::InvalidCharacteristic);
}

std::expected<void, FactoryModifyError>
apply_factory_modification(FactoryShip& dirship, const Race& race,
                           const command_t& argv) {
  if (!dirship.has_design()) {
    return std::unexpected(FactoryModifyError::NoShipDesignSpecified);
  }
  if (argv.size() < 2) {
    return std::unexpected(FactoryModifyError::NoCharacteristicSpecified);
  }

  auto& bp = dirship.blueprint_mut();
  const auto& btmpl = ship_template(bp.build_type);
  if (!btmpl.can_modify) {
    if (race.discoveries.hyperdrive) {
      if (argv[1] == "hyperdrive") {
        bp.has_hyperdrive = !bp.has_hyperdrive;
        return {};
      }
      return std::unexpected(FactoryModifyError::OnlyHyperdriveModifiable);
    }
    return std::unexpected(FactoryModifyError::CannotModifyShip);
  }

  const auto attr = parse_ship_attribute(argv[1]);
  if (!attr) {
    return std::unexpected(FactoryModifyError::InvalidCharacteristic);
  }
  switch (*attr) {
    case ShipAttr::Primary:
      if (!btmpl.has_primary()) {
        return std::unexpected(FactoryModifyError::InvalidCharacteristic);
      }
      return modify_battery(bp, argv, true);
    case ShipAttr::Secondary:
      if (!btmpl.has_secondary()) {
        return std::unexpected(FactoryModifyError::InvalidCharacteristic);
      }
      return modify_battery(bp, argv, false);
    case ShipAttr::Cew:
      if (!btmpl.has_cew) {
        return std::unexpected(FactoryModifyError::InvalidCharacteristic);
      }
      return modify_cew(bp, race, argv);
    default:
      return modify_simple_attribute(bp, race, btmpl, *attr, argv);
  }
}

std::optional<FactoryDesignStats>
finalize_factory_design_stats(FactoryShip& dirship, const Race& race,
                              const FactoryBlueprint& original_bp) {
  auto proto = dirship.instantiate_blueprint();
  const double cost0 = cost(*proto);
  if (cost0 > MAX_FACTORY_SHIP_COST) {
    dirship.blueprint_mut() = original_bp;
    return std::nullopt;
  }
  const ship_size_t design_size = dirship.recalculate_blueprint_stats(race.God);
  const auto& bp = dirship.blueprint();
  return FactoryDesignStats{
      .build_cost = bp.build_cost,
      .base_mass = bp.base_mass,
      .design_size = design_size,
      .complexity = bp.complexity,
      .race_tech = race.tech,
  };
}

}  // namespace

std::optional<FactorySpecsViewModel>
inspect_factory_specs(const FactoryShip& dirship, const Race& race) {
  if (!dirship.has_design()) {
    return std::nullopt;
  }

  const auto& bp = dirship.blueprint();
  const auto& btmpl = ship_template(bp.build_type);
  return FactorySpecsViewModel{
      .is_online = dirship.on(),
      .ship_name = std::string(btmpl.name),
      .ship_class = bp.shipclass,
      .build_cost = bp.build_cost,
      .base_mass = bp.base_mass,
      .design_size = dirship.blueprint_size(),
      .complexity = bp.complexity,
      .race_tech = race.tech,
      .armor = bp.armor,
      .max_crew = bp.max_crew,
      .max_fuel = bp.max_fuel,
      .max_resource = bp.max_resource,
      .max_hanger = bp.max_hanger,
      .max_destruct = bp.max_destruct,
      .max_speed = bp.max_speed,
      .has_primary = btmpl.has_primary(),
      .primary_battery = bp.primary_battery,
      .has_secondary = btmpl.has_secondary(),
      .secondary_battery = bp.secondary_battery,
      .can_mount = btmpl.can_mount,
      .mount = bp.mount,
      .can_hyperjump = btmpl.can_hyperjump,
      .hyperdrive = bp.has_hyperdrive,
      .can_mount_laser = btmpl.can_mount_laser,
      .laser = bp.laser,
      .has_cew = btmpl.has_cew,
      .cew = bp.cew,
      .cew_range = bp.cew_range,
  };
}

std::expected<FactoryMakeResult, FactoryMakeError>
configure_factory_blueprint(FactoryShip& dirship, const Race& race,
                            shipnum_t factory_shipno, char shipc) {
  auto i = get_build_type(shipc);
  if ((!i) || ((*i == ShipType::STYPE_POD) && (!race.pods))) {
    return std::unexpected(FactoryMakeError::IllegalShipLetter);
  }
  const auto& itmpl = ship_template(*i);
  if (itmpl.is_god_only && !race.God) {
    return std::unexpected(FactoryMakeError::GodOnlyShip);
  }
  if (!itmpl.can_be_built_by(ship_template(ShipType::OTYPE_FACTORY))) {
    return std::unexpected(FactoryMakeError::DoesNotRequireFactory);
  }

  const FactoryBlueprint original_bp = dirship.blueprint();
  dirship.configure_blueprint(*i, &race, std::format("mod {}", factory_shipno));

  auto stats = finalize_factory_design_stats(dirship, race, original_bp);
  if (!stats) {
    return std::unexpected(FactoryMakeError::MaxCostExceeded);
  }

  return FactoryMakeResult{
      .ship_name = std::string(itmpl.name),
      .stats = *stats,
  };
}

std::expected<FactoryModifyResult, FactoryModifyError>
modify_factory_attribute(FactoryShip& dirship, const Race& race,
                         const command_t& argv) {
  const FactoryBlueprint original_bp = dirship.blueprint();

  if (auto mod_ok = apply_factory_modification(dirship, race, argv); !mod_ok) {
    dirship.blueprint_mut() = original_bp;
    return std::unexpected(mod_ok.error());
  }

  auto stats = finalize_factory_design_stats(dirship, race, original_bp);
  if (!stats) {
    return std::unexpected(FactoryModifyError::MaxCostExceeded);
  }

  return FactoryModifyResult{.stats = *stats};
}

namespace {

std::expected<void, UpgradeError> upgrade_battery(Ship& ship,
                                                  const Ship& dirship,
                                                  const command_t& argv,
                                                  bool is_primary) {
  if (argv.size() < 4) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::InvalidGunCharacteristic});
  }
  const auto current_battery =
      is_primary ? dirship.primary_battery() : dirship.secondary_battery();

  if (argv[2] == "strength") {
    if (current_battery.caliber == guntype_t::NONE) {
      return std::unexpected(
          UpgradeError{.reason = UpgradeErrorReason::NoCaliberDefined});
    }
    auto parsed = parse_non_negative_int(argv[3]);
    if (!parsed) {
      return std::unexpected(
          UpgradeError{.reason = UpgradeErrorReason::RidiculousSetting});
    }
    const auto count = std::max(*parsed, current_battery.count);
    if (is_primary) {
      ship.set_primary_battery(count, current_battery.caliber);
    } else {
      ship.set_secondary_battery(count, current_battery.caliber);
    }
    return {};
  }

  if (argv[2] == "caliber") {
    auto requested = parse_caliber_name(argv[3]);
    if (!requested) {
      return std::unexpected(
          UpgradeError{.reason = UpgradeErrorReason::InvalidCaliber});
    }
    const auto max_caliber = is_primary
                                 ? shipdata_primary(dirship.build_type())
                                 : shipdata_secondary(dirship.build_type());
    const auto new_caliber =
        std::min(max_caliber, std::max(*requested, current_battery.caliber));
    if (is_primary) {
      ship.set_primary_battery(current_battery.count, new_caliber);
    } else {
      ship.set_secondary_battery(current_battery.count, new_caliber);
    }
    return {};
  }

  return std::unexpected(
      UpgradeError{.reason = UpgradeErrorReason::InvalidGunCharacteristic});
}

std::expected<void, UpgradeError> upgrade_cew(Ship& ship, const Ship& dirship,
                                              const Race& race,
                                              const command_t& argv) {
  if (!race.discoveries.cew) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::NoCewDiscovery});
  }
  if (argv.size() < 4) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::InvalidCewOption});
  }
  auto parsed = parse_non_negative_int(argv[3]);
  if (!parsed) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::RidiculousSetting});
  }
  if (argv[2] == "strength") {
    ship.cew() = std::max(dirship.cew(), *parsed);
    return {};
  }
  if (argv[2] == "range") {
    ship.cew_range() = std::max(dirship.cew_range(), *parsed);
    return {};
  }
  return std::unexpected(
      UpgradeError{.reason = UpgradeErrorReason::InvalidCewOption});
}

std::expected<void, UpgradeError>
upgrade_simple_attribute(Ship& ship, const Ship& dirship, const Race& race,
                         const ShipTemplate& btmpl, ShipAttr attr,
                         const command_t& argv) {
  switch (attr) {
    case ShipAttr::Mount:
      if (!btmpl.can_mount) break;
      if (dirship.mount()) break;
      if (!race.discoveries.crystal) {
        return std::unexpected(
            UpgradeError{.reason = UpgradeErrorReason::NoCrystalDiscovery});
      }
      ship.mount() = true;
      return {};
    case ShipAttr::Hyperdrive:
      if (!btmpl.can_hyperjump) break;
      if (dirship.hyper_drive().has) break;
      if (!race.discoveries.hyperdrive) break;
      ship.hyper_drive().has = true;
      return {};
    case ShipAttr::Laser:
      if (!btmpl.can_mount_laser) break;
      if (dirship.laser()) break;
      if (!race.discoveries.laser) {
        return std::unexpected(
            UpgradeError{.reason = UpgradeErrorReason::NoLaserDiscovery});
      }
      ship.laser() = true;
      return {};
    default:
      break;
  }

  int value = 0;
  if (argv.size() == 3) {
    auto parsed = parse_non_negative_int(argv[2]);
    if (!parsed) {
      return std::unexpected(
          UpgradeError{.reason = UpgradeErrorReason::RidiculousSetting});
    }
    value = *parsed;
  }

  switch (attr) {
    case ShipAttr::Armor:
      ship.armor() = std::max(dirship.armor(),
                              std::min<armor_t>(value, MAX_FACTORY_ARMOR));
      return {};
    case ShipAttr::Crew:
      if (!btmpl.max_crew) break;
      ship.max_crew() =
          std::max(dirship.max_crew(),
                   std::min<population_t>(value, MAX_FACTORY_CAPACITY));
      return {};
    case ShipAttr::Cargo:
      if (!btmpl.max_resource) break;
      ship.max_resource() =
          std::max(dirship.max_resource(),
                   std::min<resource_t>(value, MAX_FACTORY_CAPACITY));
      return {};
    case ShipAttr::Hangar:
      if (!btmpl.max_hangar) break;
      ship.max_hanger() =
          std::max(dirship.max_hanger(),
                   std::min<hangar_t>(value, MAX_FACTORY_CAPACITY));
      return {};
    case ShipAttr::Fuel:
      if (!btmpl.max_fuel) break;
      ship.max_fuel() = std::max(dirship.max_fuel(),
                                 std::min<fuel_t>(value, MAX_FACTORY_CAPACITY));
      return {};
    case ShipAttr::Destruct:
      if (!btmpl.max_destruct) break;
      ship.max_destruct() =
          std::max(dirship.max_destruct(),
                   std::min<resource_t>(value, MAX_FACTORY_CAPACITY));
      return {};
    case ShipAttr::Speed:
      if (!btmpl.base_speed) break;
      ship.max_speed() = std::max(
          dirship.max_speed(),
          std::clamp<speed_t>(value, MIN_FACTORY_SPEED, MAX_FACTORY_SPEED));
      return {};
    default:
      break;
  }

  return std::unexpected(
      UpgradeError{.reason = UpgradeErrorReason::InvalidCharacteristic});
}

std::expected<void, UpgradeError>
apply_upgrade_characteristic(Ship& ship, const Ship& dirship, const Race& race,
                             const ShipTemplate& btmpl, const command_t& argv) {
  const auto attr = parse_ship_attribute(argv[1]);
  if (!attr) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::InvalidCharacteristic});
  }
  switch (*attr) {
    case ShipAttr::Primary:
      if (!btmpl.has_primary()) {
        return std::unexpected(
            UpgradeError{.reason = UpgradeErrorReason::InvalidCharacteristic});
      }
      return upgrade_battery(ship, dirship, argv, true);
    case ShipAttr::Secondary:
      if (!btmpl.has_secondary()) {
        return std::unexpected(
            UpgradeError{.reason = UpgradeErrorReason::InvalidCharacteristic});
      }
      return upgrade_battery(ship, dirship, argv, false);
    case ShipAttr::Cew:
      if (!btmpl.has_cew) {
        return std::unexpected(
            UpgradeError{.reason = UpgradeErrorReason::InvalidCharacteristic});
      }
      return upgrade_cew(ship, dirship, race, argv);
    default:
      return upgrade_simple_attribute(ship, dirship, race, btmpl, *attr, argv);
  }
}

std::expected<void, UpgradeError>
check_carrier_hangar_capacity(EntityManager& entity_manager,
                              const Ship& dirship, const Ship& candidate) {
  if (dirship.whatorbits() != ScopeLevel::LEVEL_SHIP) {
    return {};
  }
  std::optional<UpgradeError> err;
  entity_manager.with_ship(*dirship.destshipno(), [&](const Ship& carrier) {
    const long available_space = static_cast<long>(carrier.max_hanger()) -
                                 (static_cast<long>(carrier.hanger()) -
                                  static_cast<long>(dirship.size()));
    const long needed_size = static_cast<long>(candidate.calculate_size());
    if (available_space < needed_size) {
      err = UpgradeError{
          .reason = UpgradeErrorReason::InsufficientCarrierHangar,
          .carrier_letter = carrier.type_letter(),
          .carrier_shipno = *dirship.destshipno(),
          .more_hangar_needed = needed_size - available_space,
      };
    }
  });
  if (err) {
    return std::unexpected(*err);
  }
  return {};
}

void commit_ship_upgrade(EntityManager& entity_manager, Ship& dirship,
                         const Ship& candidate, const Race& race,
                         resource_t netcost) {
  const ship_size_t old_size = dirship.size();
  const double old_mass = dirship.mass();
  dirship.sync_base_mass();

  dirship.armor() = candidate.armor();
  dirship.max_crew() = candidate.max_crew();
  dirship.max_resource() = candidate.max_resource();
  dirship.max_hanger() = candidate.max_hanger();
  dirship.max_fuel() = candidate.max_fuel();
  dirship.mount() = candidate.mount();
  dirship.max_destruct() = candidate.max_destruct();
  dirship.max_speed() = candidate.max_speed();
  dirship.hyper_drive() = candidate.hyper_drive();
  dirship.set_primary_battery(candidate.primary_battery());
  dirship.set_secondary_battery(candidate.secondary_battery());
  if (dirship.active_battery() == ActiveBattery::NONE) {
    if (dirship.primary_battery().has_guns()) {
      dirship.active_battery() = ActiveBattery::PRIMARY;
    } else if (dirship.secondary_battery().has_guns()) {
      dirship.active_battery() = ActiveBattery::SECONDARY;
    }
    dirship.retaliate() = dirship.active_guns();
    dirship.protect().retaliate = dirship.active_guns() > 0;
  }
  dirship.cew() = candidate.cew();
  dirship.cew_range() = candidate.cew_range();
  dirship.laser() = candidate.laser();

  dirship.recalculate_hull_stats(race.God);
  const resource_t from_ship = std::min(netcost, dirship.resource());
  const resource_t remaining = netcost - from_ship;
  dirship.consume_resource(from_ship);
  if (remaining > 0 && dirship.is_landed()) {
    entity_manager.mutate_planet(
        dirship.storbits(), dirship.pnumorbits(), [&](Planet& planet) {
          planet.info(race.Playernum).resource -= remaining;
        });
  }

  if (dirship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const shipnum_t carrier_id = *dirship.destshipno();
    double carrier_delta = 0.0;
    entity_manager.mutate_ship(carrier_id, [&](Ship& carrier) {
      const double old_carrier_mass = carrier.mass();
      if (remaining > 0) {
        carrier.consume_resource(remaining);
      }
      carrier.unload_docked_craft(old_size, old_mass);
      carrier.load_docked_craft(dirship);
      carrier_delta = carrier.mass() - old_carrier_mass;
    });
    entity_manager.propagate_ancestor_mass_delta(carrier_id, carrier_delta);
  }
}

}  // namespace

std::expected<UpgradeShipResult, UpgradeError>
upgrade_single_ship(EntityManager& entity_manager, Ship& dirship,
                    const Race& race, const command_t& argv) {
  if (dirship.damage()) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::ShipDamaged});
  }
  if (dirship.type() == ShipType::OTYPE_FACTORY) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::CannotUpgradeFactory});
  }

  const auto& btmpl = ship_template(dirship.build_type());
  if (!btmpl.can_modify) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::CannotUpgradeShip});
  }

  SimulatedShip candidate(dirship);
  if (const auto ok =
          apply_upgrade_characteristic(candidate, dirship, race, btmpl, argv);
      !ok) {
    return std::unexpected(ok.error());
  }

  const double complex = complexity(candidate);
  if (complex > race.tech && !race.God) {
    return std::unexpected(UpgradeError{
        .reason = UpgradeErrorReason::InsufficientTech,
        .required_tech = complex,
    });
  }

  if (const auto hangar_ok =
          check_carrier_hangar_capacity(entity_manager, dirship, candidate);
      !hangar_ok) {
    return std::unexpected(hangar_ok.error());
  }

  const resource_t newcost =
      race.God ? 0 : static_cast<resource_t>(cost(candidate));
  const resource_t oldcost = race.God ? 0 : dirship.build_cost();
  if (newcost < oldcost) {
    return std::unexpected(
        UpgradeError{.reason = UpgradeErrorReason::CannotDowngrade});
  }

  resource_t netcost =
      race.God ? 0 : SHIP_MODIFICATION_COST_MULTIPLIER * (newcost - oldcost);
  if (!race.God && netcost == 0) {
    netcost = 1;
  }

  resource_t available = dirship.resource();
  if (dirship.is_landed()) {
    const auto& planet =
        *entity_manager.peek_planet(dirship.storbits(), dirship.pnumorbits());
    if (planet.has_info(race.Playernum)) {
      available += planet.info(race.Playernum).resource;
    }
  } else if (dirship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const auto& carrier = *entity_manager.peek_ship(*dirship.destshipno());
    if (carrier.owner() == race.Playernum) {
      available += carrier.resource();
    }
  }

  if (netcost > available) {
    return std::unexpected(UpgradeError{
        .reason = UpgradeErrorReason::InsufficientResources,
        .old_cost = oldcost,
        .new_cost = newcost,
        .net_cost = netcost,
    });
  }

  commit_ship_upgrade(entity_manager, dirship, candidate, race, netcost);
  return UpgradeShipResult{
      .old_cost = oldcost,
      .new_cost = newcost,
      .net_cost = netcost,
  };
}
