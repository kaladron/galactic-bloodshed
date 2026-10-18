// SPDX-License-Identifier: Apache-2.0

/// \file construction.cc
/// \brief Ship construction validation, factory spawning, and shipping cost
/// mechanics implementation.

module;

import scnlib;
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
can_build_on_sector(EntityManager& entity_manager, const ShipType what,
                    const Race& race, const Planet& planet,
                    const Sector& sector, const Coordinates& c) {
  auto shipc = ship_template(what).letter;
  if (!sector.get_popn()) {
    return std::unexpected("You have no more civs in the sector!\n");
  }
  if (sector.is_wasted()) {
    return std::unexpected("You can't build on wasted sectors.\n");
  }
  if (sector.get_owner() != race.Playernum && !race.God) {
    return std::unexpected("You don't own that sector.\n");
  }
  if (!ship_template(what).can_build_on_planet() && !race.God) {
    std::string temp = std::format(
        "This ship type cannot be built on a planet.\nUse 'build ? {}' to find "
        "out where it can be built.\n",
        shipc);
    return std::unexpected(temp);
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

// Used for optional parameters.  If the element requested exists, use
// it.  If the number is negative or non-numeric, return zero instead.
int getcount(const command_t& argv, const std::size_t elem) {
  if (argv.size() <= elem) {
    return 1;
  }
  auto res = scn::scan<int>(argv[elem], "{}");
  if (!res) {
    return 0;
  }
  return std::max(res->value(), 0);
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
  for (std::size_t i = 0; i < NUMSTYPES; ++i) {
    if (ship_templates[i].letter == shipc) return ShipType{static_cast<int>(i)};
  }
  return {};
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
              const bool god, const Ship& builder) {
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
  if (builder.is_docked()) {
    return std::unexpected(ShipBuildError::ShipDocked);
  }
  if (builder.damage()) {
    return std::unexpected(ShipBuildError::ShipDamaged);
  }
  if (builder.type() == ShipType::OTYPE_FACTORY && !builder.on()) {
    return std::unexpected(ShipBuildError::FactoryNotOnline);
  }
  if (builder.type() == ShipType::OTYPE_FACTORY && !builder.is_landed()) {
    return std::unexpected(ShipBuildError::FactoryNotLanded);
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
  newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                          (newship.type() == ShipType::OTYPE_PLOW))
                             ? "5"
                             : "Standard");
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
    newship.dock_into_carrier(builder);
  }
  newship.tech() = race.tech;
  newship.set_coordinates(builder.coordinates());
  newship.set_land_coords(builder.land_coords());
  newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                          (newship.type() == ShipType::OTYPE_PLOW))
                             ? "5"
                             : "Standard");
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
  ship->build_cost() = r.God ? 0 : cost(*ship);
  return ship;
}

std::unique_ptr<Ship> getfactship(const Ship& b) {
  ship_struct data{
      .armor = b.armor(),
      .max_crew = b.max_crew(),
      .max_resource = b.max_resource(),
      .max_destruct = b.max_destruct(),
      .max_fuel = b.max_fuel(),
      .max_speed = b.max_speed(),
      .build_type = b.build_type(),
      .build_cost = b.build_cost(),
      .mount = b.mount(),
      .hyper_drive = {.has = b.hyper_drive().has},
      .cew = b.cew(),
      .cew_range = b.cew_range(),
      .laser = b.laser(),
      .type = b.build_type(),
      .guns = b.primary_battery().has_guns() ? ActiveBattery::PRIMARY
                                             : ActiveBattery::NONE,
      .primary_battery = b.primary_battery(),
      .secondary_battery = b.secondary_battery(),
      .max_hanger = b.max_hanger(),
  };

  auto s = ShipFactory::create(std::move(data));
  s->size() = s->calculate_size();
  s->set_mass(s->base_mass());
  return s;
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

  double dist = star_to->coordinates().distance_to(star_from->coordinates());

  int junk = (int)(dist / 10000.0);
  junk *= 10000;

  double factor = 1.0 - std::exp(-(double)junk / MERCHANT_LENGTH);

  money_t fcost = std::round(factor * (double)value);
  return {fcost, dist};
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

  const bool in_deep_space =
      (s.whatorbits() == ScopeLevel::LEVEL_UNIV || s.storbits() == 0);
  if (in_deep_space) {
    if (!race.God) {
      const auto* univ = em.peek_universe();
      if (univ->get_AP(race.Playernum) < 1) {
        return std::unexpected(ScrapError::InsufficientUniverseAp);
      }
      em.mutate_universe([&](Universe& u) { u.deduct_AP(race.Playernum, 1); });
    }
  } else {
    const auto* star = em.peek_star(s.storbits());
    if (!star) {
      return std::unexpected(ScrapError::StarNotFound);
    }
    if (!race.God) {
      if (star->AP(race.Playernum) < 1) {
        return std::unexpected(ScrapError::InsufficientStarAp);
      }
      em.mutate_star(s.storbits(),
                     [&](Star& st) { st.AP(race.Playernum) -= 1; });
    }
  }

  ScrapShipResult result{
      .ship_display = std::format("{}", s),
      .was_landed = s.is_landed(),
      .reclaimed = s.docked(),
  };

  if (s.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    if (const auto* tox = s.as<ToxicWasteShip>()) {
      result.toxin_released = tox->toxic_level();
    }
  }

  const bool is_landed_on_planet =
      (s.whatorbits() == ScopeLevel::LEVEL_PLAN && s.is_landed());
  player_t sect_owner = 0;
  if (is_landed_on_planet) {
    sect_owner = em.peek_sectormap(s.storbits(), s.pnumorbits())
                     ->get(s.land_coords())
                     .get_owner();
  }
  const bool foreign_sector =
      (is_landed_on_planet && sect_owner != 0 && sect_owner != race.Playernum);

  resource_t scrapval = s.effective_cost() / 2 + s.resource();
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
      if (fuelval > 0.0 && s2.fuel() + fuelval > s2.max_fuel_capacity()) {
        fuelval = std::max(0.0, s2.max_fuel_capacity() - s2.fuel());
        result.fuel_room_limit = fuelval;
      }
      if (destval > 0 && s2.destruct() + destval > s2.max_destruct_capacity()) {
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
      if (xtalval > 0 && s2.crystals() + xtalval > s2.max_crystals_capacity()) {
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
    em.mutate_ship(dest_ship, [&](Ship& s2) {
      s2.add_crystals(xtalval);
      s2.add_fuel(fuelval);
      s2.add_destruct(destval);
      s2.add_resource(scrapval);
      s2.add_troops(troopval, race.mass);
      s2.add_popn(crewval, race.mass);
    });
  }

  em.kill_ship(race.Playernum, s);

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
  auto res = scn::scan<int>(arg, "{}");
  if (!res || res->value() < 0) {
    return std::nullopt;
  }
  return res->value();
}

std::expected<void, FactoryModifyError>
modify_battery(Ship& dirship, const command_t& argv, bool is_primary) {
  if (argv.size() < 4) {
    return std::unexpected(FactoryModifyError::InvalidGunCharacteristic);
  }
  if (argv[2] == "strength") {
    auto strength = parse_non_negative_int(argv[3]);
    if (!strength) {
      return std::unexpected(FactoryModifyError::RidiculousSetting);
    }
    if (is_primary) {
      dirship.set_primary_battery(*strength, dirship.primary_battery().caliber);
    } else {
      dirship.set_secondary_battery(*strength,
                                    dirship.secondary_battery().caliber);
    }
    return {};
  }
  if (argv[2] == "caliber") {
    auto new_caliber = parse_caliber_name(argv[3]);
    if (!new_caliber) {
      return std::unexpected(FactoryModifyError::InvalidCaliber);
    }
    if (is_primary) {
      dirship.set_primary_battery(
          dirship.primary_battery().count,
          std::min(shipdata_primary(dirship.build_type()), *new_caliber));
    } else {
      dirship.set_secondary_battery(
          dirship.secondary_battery().count,
          std::min(shipdata_secondary(dirship.build_type()), *new_caliber));
    }
    return {};
  }
  return std::unexpected(FactoryModifyError::InvalidGunCharacteristic);
}

std::expected<void, FactoryModifyError>
modify_cew(Ship& dirship, const Race& race, const command_t& argv) {
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
    dirship.cew() = *value;
    return {};
  }
  if (argv[2] == "range") {
    dirship.cew_range() = *value;
    return {};
  }
  return std::unexpected(FactoryModifyError::InvalidCewOption);
}

std::expected<void, FactoryModifyError>
modify_simple_attribute(Ship& dirship, const Race& race,
                        const ShipTemplate& btmpl, const command_t& argv) {
  std::string_view attr = argv[1];
  if (attr == "mount" && btmpl.can_mount && race.discoveries.crystal) {
    dirship.mount() = !dirship.mount();
    return {};
  }
  if (attr == "hyperdrive" && btmpl.can_hyperjump &&
      race.discoveries.hyperdrive) {
    dirship.hyper_drive().has = !dirship.hyper_drive().has;
    return {};
  }
  if (attr == "laser" && btmpl.can_mount_laser) {
    if (!race.discoveries.laser) {
      return std::unexpected(FactoryModifyError::NoLaserDiscovery);
    }
    dirship.laser() = !dirship.laser();
    return {};
  }

  int value = 0;
  if (argv.size() >= 3) {
    auto parsed = parse_non_negative_int(argv[2]);
    if (!parsed) {
      return std::unexpected(FactoryModifyError::RidiculousSetting);
    }
    value = *parsed;
  }

  if (attr == "armor") {
    dirship.armor() = std::min<armor_t>(value, MAX_FACTORY_ARMOR);
    return {};
  }
  if (attr == "crew" && btmpl.max_crew) {
    dirship.max_crew() = std::min<population_t>(value, MAX_FACTORY_CAPACITY);
    return {};
  }
  if ((attr == "cargo" || attr == "resource") && btmpl.max_resource) {
    dirship.max_resource() = std::min<resource_t>(value, MAX_FACTORY_CAPACITY);
    return {};
  }
  if (attr == "hanger" && btmpl.max_hangar) {
    dirship.max_hanger() = std::min<hangar_t>(value, MAX_FACTORY_CAPACITY);
    return {};
  }
  if (attr == "fuel" && btmpl.max_fuel) {
    dirship.max_fuel() = std::min<fuel_t>(value, MAX_FACTORY_CAPACITY);
    return {};
  }
  if (attr == "destruct" && btmpl.max_destruct) {
    dirship.max_destruct() = std::min<resource_t>(value, MAX_FACTORY_CAPACITY);
    return {};
  }
  if (attr == "speed" && btmpl.base_speed) {
    dirship.max_speed() =
        std::clamp<speed_t>(value, MIN_FACTORY_SPEED, MAX_FACTORY_SPEED);
    return {};
  }

  return std::unexpected(FactoryModifyError::InvalidCharacteristic);
}

std::expected<void, FactoryModifyError>
apply_factory_modification(Ship& dirship, const Race& race,
                           const command_t& argv) {
  if (!dirship.has_factory_design()) {
    return std::unexpected(FactoryModifyError::NoShipDesignSpecified);
  }
  if (argv.size() < 2) {
    return std::unexpected(FactoryModifyError::NoCharacteristicSpecified);
  }

  const auto& btmpl = ship_template(dirship.build_type());
  if (!btmpl.can_modify) {
    if (race.discoveries.hyperdrive) {
      if (argv[1] == "hyperdrive") {
        dirship.hyper_drive().has = !dirship.hyper_drive().has;
        return {};
      }
      return std::unexpected(FactoryModifyError::OnlyHyperdriveModifiable);
    }
    return std::unexpected(FactoryModifyError::CannotModifyShip);
  }

  if (argv[1] == "primary" && btmpl.has_primary()) {
    return modify_battery(dirship, argv, true);
  }
  if (argv[1] == "secondary" && btmpl.has_secondary()) {
    return modify_battery(dirship, argv, false);
  }
  if (argv[1] == "cew" && btmpl.has_cew) {
    return modify_cew(dirship, race, argv);
  }
  return modify_simple_attribute(dirship, race, btmpl, argv);
}

std::optional<FactoryDesignStats>
finalize_factory_design_stats(Ship& dirship, const Race& race,
                              const ship_struct& original_state) {
  const double cost0 = cost(dirship);
  if (cost0 > MAX_FACTORY_SHIP_COST) {
    dirship.to_struct() = original_state;
    return std::nullopt;
  }

  dirship.build_cost() = race.God ? 0 : static_cast<resource_t>(cost0);
  dirship.size() = dirship.calculate_size();
  const ship_size_t design_size = dirship.size();
  const double base_mass = dirship.base_mass();
  dirship.complexity() = complexity(dirship);

  dirship.size() = original_state.size;
  return FactoryDesignStats{
      .build_cost = dirship.build_cost(),
      .base_mass = base_mass,
      .design_size = design_size,
      .complexity = dirship.complexity(),
      .race_tech = race.tech,
  };
}

}  // namespace

std::optional<FactorySpecsViewModel> inspect_factory_specs(Ship& dirship,
                                                           const Race& race) {
  if (!dirship.has_factory_design()) {
    return std::nullopt;
  }

  const ship_size_t original_size = dirship.size();
  dirship.size() = dirship.calculate_size();
  const ship_size_t design_size = dirship.size();
  const double base_mass = dirship.base_mass();
  dirship.size() = original_size;

  const auto& btmpl = ship_template(dirship.build_type());
  return FactorySpecsViewModel{
      .is_online = dirship.on(),
      .ship_name = std::string(btmpl.name),
      .ship_class = dirship.shipclass(),
      .build_cost = dirship.build_cost(),
      .base_mass = base_mass,
      .design_size = design_size,
      .complexity = dirship.complexity(),
      .race_tech = race.tech,
      .armor = dirship.armor(),
      .max_crew = dirship.max_crew(),
      .max_fuel = dirship.max_fuel(),
      .max_resource = dirship.max_resource(),
      .max_hanger = dirship.max_hanger(),
      .max_destruct = dirship.max_destruct(),
      .max_speed = dirship.max_speed(),
      .has_primary = btmpl.has_primary(),
      .primary_battery = dirship.primary_battery(),
      .has_secondary = btmpl.has_secondary(),
      .secondary_battery = dirship.secondary_battery(),
      .can_mount = btmpl.can_mount,
      .mount = dirship.mount(),
      .can_hyperjump = btmpl.can_hyperjump,
      .hyperdrive = dirship.hyper_drive().has,
      .can_mount_laser = btmpl.can_mount_laser,
      .laser = dirship.laser(),
      .has_cew = btmpl.has_cew,
      .cew = dirship.cew(),
      .cew_range = dirship.cew_range(),
  };
}

std::expected<FactoryMakeResult, FactoryMakeError>
configure_factory_blueprint(Ship& dirship, const Race& race,
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

  const ship_struct original_state = dirship.to_struct();
  dirship.set_factory_blueprint(*i, &race);
  dirship.shipclass() = std::format("mod {}", factory_shipno);

  auto stats = finalize_factory_design_stats(dirship, race, original_state);
  if (!stats) {
    return std::unexpected(FactoryMakeError::MaxCostExceeded);
  }

  return FactoryMakeResult{
      .ship_name = std::string(itmpl.name),
      .stats = *stats,
  };
}

std::expected<FactoryModifyResult, FactoryModifyError>
modify_factory_attribute(Ship& dirship, const Race& race,
                         const command_t& argv) {
  const ship_struct original_state = dirship.to_struct();
  dirship.size() = dirship.calculate_size();

  if (auto mod_ok = apply_factory_modification(dirship, race, argv); !mod_ok) {
    dirship.to_struct() = original_state;
    return std::unexpected(mod_ok.error());
  }

  auto stats = finalize_factory_design_stats(dirship, race, original_state);
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
                         const ShipTemplate& btmpl, const command_t& argv) {
  const std::string_view attr = argv[1];
  if (attr == "mount" && btmpl.can_mount && !dirship.mount()) {
    if (!race.discoveries.crystal) {
      return std::unexpected(
          UpgradeError{.reason = UpgradeErrorReason::NoCrystalDiscovery});
    }
    ship.mount() = true;
    return {};
  }
  if (attr == "hyperdrive" && btmpl.can_hyperjump &&
      !dirship.hyper_drive().has && race.discoveries.hyperdrive) {
    ship.hyper_drive().has = true;
    return {};
  }
  if (attr == "laser" && btmpl.can_mount_laser) {
    if (!race.discoveries.laser) {
      return std::unexpected(
          UpgradeError{.reason = UpgradeErrorReason::NoLaserDiscovery});
    }
    ship.laser() = true;
    return {};
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

  if (attr == "armor") {
    ship.armor() =
        std::max(dirship.armor(), std::min<armor_t>(value, MAX_FACTORY_ARMOR));
    return {};
  }
  if (attr == "crew" && btmpl.max_crew) {
    ship.max_crew() =
        std::max(dirship.max_crew(),
                 std::min<population_t>(value, MAX_FACTORY_CAPACITY));
    return {};
  }
  if ((attr == "cargo" || attr == "resource") && btmpl.max_resource) {
    ship.max_resource() =
        std::max(dirship.max_resource(),
                 std::min<resource_t>(value, MAX_FACTORY_CAPACITY));
    return {};
  }
  if (attr == "hanger" && btmpl.max_hangar) {
    ship.max_hanger() = std::max(
        dirship.max_hanger(), std::min<hangar_t>(value, MAX_FACTORY_CAPACITY));
    return {};
  }
  if (attr == "fuel" && btmpl.max_fuel) {
    ship.max_fuel() = std::max(dirship.max_fuel(),
                               std::min<fuel_t>(value, MAX_FACTORY_CAPACITY));
    return {};
  }
  if (attr == "destruct" && btmpl.max_destruct) {
    ship.max_destruct() =
        std::max(dirship.max_destruct(),
                 std::min<resource_t>(value, MAX_FACTORY_CAPACITY));
    return {};
  }
  if (attr == "speed" && btmpl.base_speed) {
    ship.max_speed() = std::max(
        dirship.max_speed(),
        std::clamp<speed_t>(value, MIN_FACTORY_SPEED, MAX_FACTORY_SPEED));
    return {};
  }

  return std::unexpected(
      UpgradeError{.reason = UpgradeErrorReason::InvalidCharacteristic});
}

std::expected<void, UpgradeError>
apply_upgrade_characteristic(Ship& ship, const Ship& dirship, const Race& race,
                             const ShipTemplate& btmpl, const command_t& argv) {
  const std::string_view attr = argv[1];
  if (attr == "primary" && btmpl.has_primary()) {
    return upgrade_battery(ship, dirship, argv, true);
  }
  if (attr == "secondary" && btmpl.has_secondary()) {
    return upgrade_battery(ship, dirship, argv, false);
  }
  if (attr == "cew" && btmpl.has_cew) {
    return upgrade_cew(ship, dirship, race, argv);
  }
  return upgrade_simple_attribute(ship, dirship, race, btmpl, argv);
}

std::expected<void, UpgradeError>
check_carrier_hangar_capacity(EntityManager& entity_manager,
                              const Ship& dirship, const Ship& candidate) {
  if (dirship.whatorbits() != ScopeLevel::LEVEL_SHIP || !dirship.destshipno()) {
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
  const double old_base_mass = dirship.base_mass();

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
  }
  dirship.cew() = candidate.cew();
  dirship.cew_range() = candidate.cew_range();
  dirship.laser() = candidate.laser();

  dirship.size() = dirship.calculate_size();
  const double new_base_mass = dirship.base_mass();
  dirship.to_struct().base_mass = new_base_mass;
  dirship.set_mass(dirship.mass() + (new_base_mass - old_base_mass));
  dirship.consume_resource(netcost);
  dirship.build_cost() = race.God ? 0 : static_cast<resource_t>(cost(dirship));
  dirship.complexity() = complexity(dirship);

  if (dirship.whatorbits() == ScopeLevel::LEVEL_SHIP && dirship.destshipno()) {
    entity_manager.mutate_ship(*dirship.destshipno(), [&](Ship& carrier) {
      carrier.unload_docked_craft(old_size, old_mass);
      carrier.load_docked_craft(dirship);
    });
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

  Ship candidate(dirship.get_struct());
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

  resource_t netcost = race.God ? 0 : 2 * (newcost - oldcost);
  if (!race.God && netcost == 0) {
    netcost = 1;
  }

  if (netcost > dirship.resource()) {
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
