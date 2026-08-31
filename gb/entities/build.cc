// SPDX-License-Identifier: Apache-2.0

module;

import std;
#undef stdout

module gblib;

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
    const ShipList shiplist(entity_manager, planet.ships());
    for (const Ship& s : shiplist) {
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
  if (planet.slaved_to() != 0 && planet.slaved_to() != Playernum) {
    std::string message = std::format("This planet is enslaved by player {}.\n",
                                      planet.slaved_to());
    push_telegram(g.entity_manager, Playernum, Governor, message);
    return false;
  }
  if (Governor != 0 && star.governor(Playernum) != Governor) {
    g.out << "You are not authorized in this system.\n";
    return false;
  }
  return true;
}

std::optional<ShipType> get_build_type(const char shipc) {
  for (int i = 0; i < std::extent<decltype(Shipltrs)>::value; ++i) {
    if (Shipltrs[i] == shipc) return ShipType{i};
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

std::optional<ScopeLevel> build_at_ship(GameObj& g, Ship* builder,
                                        starnum_t* snum, planetnum_t* pnum) {
  if (testship(*builder, g)) return {};
  if (!builder->can_construct_ships()) {
    g.out << "This ship cannot construct other ships.\n";
    return {};
  }
  if (!builder->popn()) {
    g.out << "This ship has no crew.\n";
    return {};
  }
  if (builder->is_docked()) {
    g.out << "Undock this ship first.\n";
    return {};
  }
  if (builder->damage()) {
    g.out << "This ship is damaged and cannot build.\n";
    return {};
  }
  if (builder->type() == ShipType::OTYPE_FACTORY && !builder->on()) {
    g.out << "This factory is not online.\n";
    return {};
  }
  if (builder->type() == ShipType::OTYPE_FACTORY && !builder->is_landed()) {
    g.out << "Factories must be landed on a planet.\n";
    return {};
  }
  *snum = builder->storbits();
  *pnum = builder->pnumorbits();
  return (builder->whatorbits());
}

void autoload_at_planet(player_t Playernum, Ship* s, Planet* planet,
                        Sector& sector, int* crew, double* fuel) {
  *crew = std::min(s->max_crew_capacity(), sector.get_popn());
  *fuel = std::min(static_cast<double>(s->max_fuel_capacity()),
                   static_cast<double>(planet->info(Playernum).fuel));
  sector.subtract_popn(*crew);
  if (!sector.get_popn() && !sector.get_troops()) sector.set_owner(0);
  planet->info(Playernum).fuel -= (int)(*fuel);
}

void autoload_at_ship(Ship* s, Ship* b, int* crew, double* fuel) {
  *crew = std::min(s->max_crew_capacity(), b->popn());
  *fuel = std::min(static_cast<double>(s->max_fuel_capacity()), b->fuel());
  b->popn() -= *crew;
  b->fuel() -= *fuel;
}

void initialize_new_ship(GameObj& g, const Race& race, Ship* newship,
                         double load_fuel, int load_crew) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  newship->speed() = newship->max_speed_capacity();
  newship->owner() = Playernum;
  newship->governor() = Governor;
  newship->fuel() = race.God ? newship->max_fuel_capacity() : load_fuel;
  newship->popn() = race.God ? newship->max_crew_capacity() : load_crew;
  newship->troops() = 0;
  newship->resource() = race.God ? newship->max_resource_capacity() : 0;
  newship->destruct() = race.God ? newship->max_destruct_capacity() : 0;
  newship->crystals() = 0;
  newship->hanger() = 0;
  newship->mass() = newship->base_mass() + (double)newship->popn() * race.mass +
                    newship->fuel() * MASS_FUEL +
                    (double)newship->resource() * MASS_RESOURCE +
                    (double)newship->destruct() * MASS_DESTRUCT;
  newship->alive() = 1;
  newship->active() = 1;
  newship->protect().self = newship->active_guns() > 0;
  newship->hyper_drive().on = false;
  newship->hyper_drive().charge = 0;
  newship->mounted() = race.God ? newship->mount() : 0;
  newship->cloak() = 0;
  newship->cloaked() = 0;
  newship->fire_laser() = 0;
  newship->mode() = 0;
  newship->rad() = 0;
  newship->damage() = race.God ? 0 : newship->get_template().base_damage;
  newship->retaliate() = newship->primary();
  newship->ships() = 0;
  newship->on() = 0;
  switch (newship->type()) {
    case ShipType::OTYPE_VN:
      if (auto* vn = newship->as<VonNeumannShip>()) {
        vn->mind() = MindData{.progenitor = Playernum,
                              .target = 0,
                              .generation = 1,
                              .busy = 1,
                              .tampered = 0,
                              .who_killed = 0};
      }
      break;
    case ShipType::STYPE_MINE:
      if (auto* mine = newship->as<MineShip>()) {
        mine->set_trigger_radius(100);
      }
      g.out << "Mine disarmed.\nTrigger radius set at 100.\n";
      break;
    case ShipType::OTYPE_TRANSDEV:
      if (auto* trans = newship->as<TransporterShip>()) {
        trans->set_target_ship(shipnum_t{0});
      }
      newship->on() = 0;
      g.out << "Receive OFF.  Change with order.\n";
      break;
    case ShipType::OTYPE_AP:
      g.out << "Processor OFF.\n";
      break;
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE:
      g.out << std::format("Telescope range is {:.2f}.\n",
                           tele_range(newship->type(), newship->tech()));
      break;
    default:
      break;
  }
  if (newship->damage()) {
    g.out << std::format(
        "Warning: This ship is constructed with a {}% damage level.\n",
        newship->damage());
    if (!newship->can_repair() && newship->max_crew_capacity())
      g.out << "It will need resources to become fully operational.\n";
  }
  if (newship->can_repair() && newship->max_crew_capacity())
    g.out << "This ship does not need resources to repair.\n";
  if (newship->type() == ShipType::OTYPE_FACTORY)
    g.out
        << "This factory may not begin repairs until it has been activated.\n";
  if (!newship->max_crew_capacity())
    g.out << "This ship is robotic, and may not repair itself.\n";

  g.out << std::format("Loaded with {} crew and {:.1f} fuel.\n", load_crew,
                       load_fuel);
}

void create_ship_by_planet(EntityManager& entity_manager, player_t Playernum,
                           governor_t Governor, const Race& race, Ship& newship,
                           Planet& planet, starnum_t snum, planetnum_t pnum,
                           Coordinates land_coords) {
  shipnum_t shipno;

  newship.tech() = race.tech;
  const auto& star = *entity_manager.peek_star(snum);
  newship.xpos() = star.xpos() + planet.xpos();
  newship.ypos() = star.ypos() + planet.ypos();
  newship.set_land_coords(land_coords);
  newship.shipclass() = (((newship.type() == ShipType::OTYPE_TERRA) ||
                          (newship.type() == ShipType::OTYPE_PLOW))
                             ? "5"
                             : "Standard");
  newship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  newship.whatdest() = ScopeLevel::LEVEL_PLAN;
  newship.deststar() = snum;
  newship.destpnum() = pnum;
  newship.storbits() = snum;
  newship.pnumorbits() = pnum;
  newship.docked() = 1;
  planet.info(Playernum).resource -= newship.build_cost();

  // Ship number will be assigned by EntityManager when created
  shipno = shipnum_t{entity_manager.num_ships().value + 1};
  newship.number() = shipno;
  newship.owner() = Playernum;
  newship.governor() = Governor;
  newship.ships() = 0;
  newship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  if (auto* waste_ship = newship.as<ToxicWasteShip>()) {
    std::string message = std::format("Toxin concentration on planet was {}%,",
                                      planet.conditions(TOXIC));
    push_telegram(entity_manager, Playernum, Governor, message);
    const auto toxic_amount =
        static_cast<unsigned char>(std::min(TOXMAX, planet.conditions(TOXIC)));
    waste_ship->set_toxic_level(toxic_amount);
    planet.conditions(TOXIC) -= toxic_amount;
    std::string toxMsg = std::format(" now {}%.\n", planet.conditions(TOXIC));
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
                         Ship* newship, Ship* builder) {
  // Ship number will be assigned by EntityManager when created
  shipnum_t shipno = shipnum_t{entity_manager.num_ships().value + 1};
  newship->number() = shipno;
  newship->owner() = Playernum;
  newship->governor() = Governor;
  if (outside) {
    newship->whatorbits() = builder->whatorbits();
    newship->whatdest() = ScopeLevel::LEVEL_UNIV;
    newship->deststar() = builder->deststar();
    newship->destpnum() = builder->destpnum();
    newship->storbits() = builder->storbits();
    newship->pnumorbits() = builder->pnumorbits();
    newship->docked() = 0;
  } else {
    newship->whatorbits() = ScopeLevel::LEVEL_SHIP;
    newship->whatdest() = ScopeLevel::LEVEL_SHIP;
    newship->deststar() = builder->deststar();
    newship->destpnum() = builder->destpnum();
    newship->destshipno() = builder->number();
    newship->storbits() = builder->storbits();
    newship->pnumorbits() = builder->pnumorbits();
    newship->docked() = 1;
  }
  newship->tech() = race.tech;
  newship->xpos() = builder->xpos();
  newship->ypos() = builder->ypos();
  newship->set_land_coords(builder->land_coords());
  newship->shipclass() = (((newship->type() == ShipType::OTYPE_TERRA) ||
                           (newship->type() == ShipType::OTYPE_PLOW))
                              ? "5"
                              : "Standard");
  builder->resource() -= newship->build_cost();

  std::string message = std::format("{} built at a cost of {} resources.\n",
                                    *newship, newship->build_cost());
  push_telegram(entity_manager, Playernum, Governor, message);

  std::string techMsg = std::format("Technology {:.1f}.\n", newship->tech());
  push_telegram(entity_manager, Playernum, Governor, techMsg);
}

void Getship(Ship* s, ShipType i, const Race& r) {
  const auto& tmpl = ship_template(i);
  ship_struct data{
      .armor = tmpl.base_armor,
      .max_crew = tmpl.max_crew,
      .max_resource = tmpl.max_cargo,
      .max_destruct = tmpl.max_destruct,
      .max_fuel = tmpl.max_fuel,
      .max_speed = tmpl.base_speed,
      .build_type = i,
      .mount = r.God && tmpl.can_mount,
      .hyper_drive = {.has = r.God && tmpl.can_hyperjump},
      .laser = r.God && tmpl.max_lasers != 0,
      .type = i,
      .guns = tmpl.primary_power ? ActiveBattery::PRIMARY : ActiveBattery::NONE,
      .primary = tmpl.max_guns,
      .primtype = shipdata_primary(i),
      .sectype = shipdata_secondary(i),
      .max_hanger = tmpl.max_hangar,
  };
  if (i == ShipType::OTYPE_VN || i == ShipType::OTYPE_BERS) {
    data.special = MindData{.progenitor = r.Playernum};
  }

  *s = std::move(*ShipFactory::create(std::move(data)));
  s->size() = ship_size(*s);
  s->base_mass() = getmass(*s);
  s->mass() = getmass(*s);
  s->build_cost() = r.God ? 0 : (int)cost(*s);
}

Ship Getfactship(const Ship& b) {
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
      .guns = b.primary() ? ActiveBattery::PRIMARY : ActiveBattery::NONE,
      .primary = b.primary(),
      .primtype = b.primtype(),
      .max_hanger = b.max_hanger(),
  };
  data.secondary = b.secondary();
  data.sectype = b.sectype();

  Ship s(data);
  s.size() = ship_size(s);
  s.base_mass() = getmass(s);
  s.mass() = getmass(s);
  return s;
}

int Shipcost(ShipType i, const Race& r) {
  Ship s;

  Getship(&s, i, r);
  return ((int)cost(s));
}

std::tuple<money_t, double> shipping_cost(EntityManager& em, const starnum_t to,
                                          const starnum_t from,
                                          const money_t value) {
  const auto* star_to = em.peek_star(to);
  const auto* star_from = em.peek_star(from);

  double dist = std::hypot(star_to->xpos() - star_from->xpos(),
                           star_to->ypos() - star_from->ypos());

  int junk = (int)(dist / 10000.0);
  junk *= 10000;

  double factor = 1.0 - std::exp(-(double)junk / MERCHANT_LENGTH);

  money_t fcost = std::round(factor * (double)value);
  return {fcost, dist};
}
