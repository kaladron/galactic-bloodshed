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
      if (s.alive() && s.type() == ShipType::OTYPE_QUARRY &&
          s.land_coords() == c) {
        return std::unexpected("There already is a quarry here.\n");
      }
    }
  }
  return {};
}

// Used for optional parameters.  If the element requested exists, use
// it.  If the number is negative, return zero instead.
int getcount(const command_t& argv, const std::size_t elem) {
  int count = argv.size() > elem ? std::stoi(argv[elem]) : 1;
  return std::max(count, 0);
}

bool can_build_at_planet(GameObj& g, const Star& star, const Planet& planet) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  if (planet.is_enslaved_to_foreign(Playernum)) {
    std::string message = std::format("This planet is enslaved by player {}.\n",
                                      *planet.slaved_to());
    push_telegram(g.entity_manager, Playernum, Governor, message);
    return false;
  }
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized in this system.\n";
    return false;
  }
  return true;
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

std::optional<ScopeLevel> build_at_ship(GameObj& g, const Ship& builder,
                                        starnum_t& snum, planetnum_t& pnum) {
  if (!g.check_commandable(builder)) return {};
  if (!builder.can_construct_ships()) {
    g.out << "This ship cannot construct other ships.\n";
    return {};
  }
  if (!builder.popn()) {
    g.out << "This ship has no crew.\n";
    return {};
  }
  if (builder.is_docked()) {
    g.out << "Undock this ship first.\n";
    return {};
  }
  if (builder.damage()) {
    g.out << "This ship is damaged and cannot build.\n";
    return {};
  }
  if (builder.type() == ShipType::OTYPE_FACTORY && !builder.on()) {
    g.out << "This factory is not online.\n";
    return {};
  }
  if (builder.type() == ShipType::OTYPE_FACTORY && !builder.is_landed()) {
    g.out << "Factories must be landed on a planet.\n";
    return {};
  }
  snum = builder.storbits();
  pnum = builder.pnumorbits();
  return builder.whatorbits();
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

namespace {

void report_new_ship_status(GameObj& g, const Ship& newship,
                            population_t load_crew, fuel_t load_fuel) {
  switch (newship.type()) {
    case ShipType::STYPE_MINE:
      g.out << "Mine disarmed.\nTrigger radius set at 100.\n";
      break;
    case ShipType::OTYPE_TRANSDEV:
      g.out << "Receive OFF.  Change with order.\n";
      break;
    case ShipType::OTYPE_AP:
      g.out << "Processor OFF.\n";
      break;
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE:
      g.out << std::format("Telescope range is {:.2f}.\n",
                           newship.tele_range());
      break;
    default:
      break;
  }
  if (newship.damage()) {
    g.out << std::format(
        "Warning: This ship is constructed with a {}% damage level.\n",
        newship.damage());
    if (!newship.can_repair() && newship.max_crew_capacity())
      g.out << "It will need resources to become fully operational.\n";
  }
  if (newship.can_repair() && newship.max_crew_capacity())
    g.out << "This ship does not need resources to repair.\n";
  if (newship.type() == ShipType::OTYPE_FACTORY)
    g.out
        << "This factory may not begin repairs until it has been activated.\n";
  if (!newship.max_crew_capacity())
    g.out << "This ship is robotic, and may not repair itself.\n";

  g.out << std::format("Loaded with {} crew and {:.1f} fuel.\n", load_crew,
                       load_fuel);
}

}  // namespace

void initialize_new_ship(GameObj& g, const Race& race, Ship& newship,
                         fuel_t load_fuel, population_t load_crew) {
  newship.initialize_constructed_state(race, g.governor(), load_fuel,
                                       load_crew);
  report_new_ship_status(g, newship, load_crew, load_fuel);
}

void create_ship_by_planet(EntityManager& entity_manager, player_t Playernum,
                           governor_t Governor, const Race& race, Ship& newship,
                           Planet& planet, starnum_t snum, planetnum_t pnum,
                           Coordinates land_coords) {
  shipnum_t shipno;

  newship.tech() = race.tech;
  const auto& star = *entity_manager.peek_star(snum);
  newship.set_coordinates(planet.absolute_coordinates(star));
  newship.set_land_coords(land_coords);
  newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                          (newship.type() == ShipType::OTYPE_PLOW))
                             ? "5"
                             : "Standard");
  newship.land_on_planet();
  newship.deststar() = snum;
  newship.destpnum() = pnum;
  newship.storbits() = snum;
  newship.pnumorbits() = pnum;
  planet.info(Playernum).resource -= newship.build_cost();

  // Ship number will be assigned by EntityManager when created
  shipno = entity_manager.next_available_ship_number();
  newship.number() = shipno;
  newship.owner() = Playernum;
  newship.governor() = Governor;
  if (auto* waste_ship = newship.as<ToxicWasteShip>()) {
    std::string message =
        std::format("Toxin concentration on planet was {}%,", planet.toxic());
    push_telegram(entity_manager, Playernum, Governor, message);
    const int toxic_amount = std::min(TOXMAX, static_cast<int>(planet.toxic()));
    waste_ship->set_toxic_level(toxic_amount);
    planet.toxic() -= toxic_amount;
    std::string toxMsg = std::format(" now {}%.\n", planet.toxic());
    push_telegram(entity_manager, Playernum, Governor, toxMsg);
  }
  std::string message = std::format("{} built at a cost of {} resources.\n",
                                    newship, newship.build_cost());
  push_telegram(entity_manager, Playernum, Governor, message);

  std::string techMsg = std::format("Technology {:.1f}.\n", newship.tech());
  push_telegram(entity_manager, Playernum, Governor, techMsg);

  std::string locMsg =
      std::format("{} is on sector {}.\n", newship, newship.land_coords());
  push_telegram(entity_manager, Playernum, Governor, locMsg);
}

void create_ship_by_ship(EntityManager& entity_manager, player_t Playernum,
                         governor_t Governor, const Race& race, bool outside,
                         Ship& newship, Ship& builder) {
  // Ship number will be assigned by EntityManager when created
  shipnum_t shipno = entity_manager.next_available_ship_number();
  newship.number() = shipno;
  newship.owner() = Playernum;
  newship.governor() = Governor;
  if (outside) {
    newship.launch_to_orbit(builder.whatorbits());
    newship.whatdest() = ScopeLevel::LEVEL_UNIV;
    newship.deststar() = builder.deststar();
    newship.destpnum() = builder.destpnum();
    newship.storbits() = builder.storbits();
    newship.pnumorbits() = builder.pnumorbits();
  } else {
    newship.dock_into_carrier(builder.number());
    newship.deststar() = builder.deststar();
    newship.destpnum() = builder.destpnum();
    newship.storbits() = builder.storbits();
    newship.pnumorbits() = builder.pnumorbits();
  }
  newship.tech() = race.tech;
  newship.set_coordinates(builder.coordinates());
  newship.set_land_coords(builder.land_coords());
  newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                          (newship.type() == ShipType::OTYPE_PLOW))
                             ? "5"
                             : "Standard");
  builder.consume_resource(newship.build_cost());

  std::string message = std::format("{} built at a cost of {} resources.\n",
                                    newship, newship.build_cost());
  push_telegram(entity_manager, Playernum, Governor, message);

  std::string techMsg = std::format("Technology {:.1f}.\n", newship.tech());
  push_telegram(entity_manager, Playernum, Governor, techMsg);
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
