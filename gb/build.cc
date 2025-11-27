// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

#include <strings.h>

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
can_build_on_sector(EntityManager& entity_manager, const int what,
                    const Race& race, const Planet& planet,
                    const Sector& sector, const Coordinates& c) {
  auto shipc = Shipltrs[what];
  if (!sector.get_popn()) {
    return std::unexpected("You have no more civs in the sector!\n");
  }
  if (sector.get_condition() == SectorType::SEC_WASTED) {
    return std::unexpected("You can't build on wasted sectors.\n");
  }
  if (sector.get_owner() != race.Playernum && !race.God) {
    return std::unexpected("You don't own that sector.\n");
  }
  if ((!(Shipdata[what][ABIL_BUILD] & 1)) && !race.God) {
    std::string temp = std::format(
        "This ship type cannot be built on a planet.\nUse 'build ? {}' to find "
        "out where it can be built.\n",
        shipc);
    return std::unexpected(temp);
  }
  if (what == ShipType::OTYPE_QUARRY) {
    const ShipList shiplist(entity_manager, planet.ships());
    for (const Ship* s : shiplist) {
      if (s->alive && s->type == ShipType::OTYPE_QUARRY && s->land_x == c.x &&
          s->land_y == c.y) {
        return std::unexpected("There already is a quarry here.\n");
      }
    }
  }
  return {};
}

// Used for optional parameters.  If the element requested exists, use
// it.  If the number is negative, return zero instead.
int getcount(const command_t& argv, const size_t elem) {
  int count = argv.size() > elem ? std::stoi(argv[elem]) : 1;
  return std::max(count, 0);
}

bool can_build_at_planet(GameObj& g, const Star& star, const Planet& planet) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (planet.slaved_to() && planet.slaved_to() != Playernum) {
    std::string message = std::format("This planet is enslaved by player {}.\n",
                                      planet.slaved_to());
    notify(Playernum, Governor, message);
    return false;
  }
  if (Governor && star.governor(Playernum - 1) != Governor) {
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
  if (!Shipdata[what][ABIL_PROGRAMMED]) {
    return std::unexpected("This ship type has not been programmed.\n");
  }
  if (Shipdata[what][ABIL_GOD] && !race.God) {
    return std::unexpected("Only Gods can build this type of ship.\n");
  }
  if (what == ShipType::OTYPE_VN && !Vn(race)) {
    return std::unexpected("You have not discovered VN technology.\n");
  }
  if (what == ShipType::OTYPE_TRANSDEV && !Avpm(race)) {
    return std::unexpected("You have not discovered AVPM technology.\n");
  }
  if (Shipdata[what][ABIL_TECH] > race.tech && !race.God) {
    std::string error = std::format(
        "You are not advanced enough to build this ship.\n%.1f engineering "
        "technology needed. You have %.1f.\n",
        (double)Shipdata[what][ABIL_TECH], race.tech);
    return std::unexpected(error);
  }
  return {};
}

std::expected<void, std::string>
can_build_on_ship(ShipType what, const Race& race, const Ship& builder) {
  if (!(Shipdata[what][ABIL_BUILD] & Shipdata[builder.type][ABIL_CONSTRUCT]) &&
      !race.God) {
    std::string error = std::format(
        "This ship type cannot be built by a {}.\nUse 'build ? {}' to find out "
        "where it can be built.\n",
        Shipnames[builder.type], Shipltrs[what]);
    return std::unexpected(error);
  }
  return {};
}

std::optional<ScopeLevel> build_at_ship(GameObj& g, Ship* builder,
                                        starnum_t* snum, planetnum_t* pnum) {
  if (testship(*builder, g)) return {};
  if (!Shipdata[builder->type][ABIL_CONSTRUCT]) {
    g.out << "This ship cannot construct other ships.\n";
    return {};
  }
  if (!builder->popn) {
    g.out << "This ship has no crew.\n";
    return {};
  }
  if (docked(*builder)) {
    g.out << "Undock this ship first.\n";
    return {};
  }
  if (builder->damage) {
    g.out << "This ship is damaged and cannot build.\n";
    return {};
  }
  if (builder->type == ShipType::OTYPE_FACTORY && !builder->on) {
    g.out << "This factory is not online.\n";
    return {};
  }
  if (builder->type == ShipType::OTYPE_FACTORY && !landed(*builder)) {
    g.out << "Factories must be landed on a planet.\n";
    return {};
  }
  *snum = builder->storbits;
  *pnum = builder->pnumorbits;
  return (builder->whatorbits);
}

void autoload_at_planet(int Playernum, Ship* s, Planet* planet, Sector& sector,
                        int* crew, double* fuel) {
  *crew = MIN(s->max_crew, sector.get_popn());
  *fuel = MIN((double)s->max_fuel, (double)planet->info(Playernum - 1).fuel);
  sector.set_popn(sector.get_popn() - *crew);
  if (!sector.get_popn() && !sector.get_troops()) sector.set_owner(0);
  planet->info(Playernum - 1).fuel -= (int)(*fuel);
}

void autoload_at_ship(Ship* s, Ship* b, int* crew, double* fuel) {
  *crew = MIN(s->max_crew, b->popn);
  *fuel = MIN((double)s->max_fuel, b->fuel);
  b->popn -= *crew;
  b->fuel -= *fuel;
}

void initialize_new_ship(GameObj& g, const Race& race, Ship* newship,
                         double load_fuel, int load_crew) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  newship->speed = newship->max_speed;
  newship->owner = Playernum;
  newship->governor = Governor;
  newship->fuel = race.God ? newship->max_fuel : load_fuel;
  newship->popn = race.God ? newship->max_crew : load_crew;
  newship->troops = 0;
  newship->resource = race.God ? newship->max_resource : 0;
  newship->destruct = race.God ? newship->max_destruct : 0;
  newship->crystals = 0;
  newship->hanger = 0;
  newship->mass = newship->base_mass + (double)newship->popn * race.mass +
                  newship->fuel * MASS_FUEL +
                  (double)newship->resource * MASS_RESOURCE +
                  (double)newship->destruct * MASS_DESTRUCT;
  newship->alive = 1;
  newship->active = 1;
  newship->protect.self = newship->guns ? 1 : 0;
  newship->hyper_drive.on = 0;
  newship->hyper_drive.ready = 0;
  newship->hyper_drive.charge = 0;
  newship->mounted = race.God ? newship->mount : 0;
  newship->cloak = 0;
  newship->cloaked = 0;
  newship->fire_laser = 0;
  newship->mode = 0;
  newship->rad = 0;
  newship->damage = race.God ? 0 : Shipdata[newship->type][ABIL_DAMAGE];
  newship->retaliate = newship->primary;
  newship->ships = 0;
  newship->on = 0;
  switch (newship->type) {
    case ShipType::OTYPE_VN:
      newship->special =
          MindData{.progenitor = static_cast<unsigned char>(Playernum),
                   .target = 0,
                   .generation = 1,
                   .busy = 1,
                   .tampered = 0,
                   .who_killed = 0};
      break;
    case ShipType::STYPE_MINE:
      newship->special = TriggerData{.radius = 100}; /* trigger radius */
      g.out << "Mine disarmed.\nTrigger radius set at 100.\n";
      break;
    case ShipType::OTYPE_TRANSDEV:
      newship->special = TransportData{.target = 0};
      newship->on = 0;
      g.out << "Receive OFF.  Change with order.\n";
      break;
    case ShipType::OTYPE_AP:
      g.out << "Processor OFF.\n";
      break;
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE: {
      std::string message =
          std::format("Telescope range is {:.2f}.\n",
                      tele_range(newship->type, newship->tech));
      notify(Playernum, Governor, message);
    } break;
    default:
      break;
  }
  if (newship->damage) {
    std::string message = std::format(
        "Warning: This ship is constructed with a {}% damage level.\n",
        newship->damage);
    notify(Playernum, Governor, message);
    if (!Shipdata[newship->type][ABIL_REPAIR] && newship->max_crew)
      notify(Playernum, Governor,
             "It will need resources to become fully operational.\n");
  }
  if (Shipdata[newship->type][ABIL_REPAIR] && newship->max_crew)
    g.out << "This ship does not need resources to repair.\n";
  if (newship->type == ShipType::OTYPE_FACTORY)
    g.out
        << "This factory may not begin repairs until it has been activated.\n";
  if (!newship->max_crew)
    g.out << "This ship is robotic, and may not repair itself.\n";

  std::string message = std::format("Loaded with {} crew and {:.1f} fuel.\n",
                                    load_crew, load_fuel);
  notify(Playernum, Governor, message);
}

void create_ship_by_planet(int Playernum, int Governor, const Race& race,
                           Ship& newship, Planet& planet, int snum, int pnum,
                           int x, int y) {
  int shipno;

  newship.tech = race.tech;
  newship.xpos = stars[snum].xpos() + planet.xpos();
  newship.ypos = stars[snum].ypos() + planet.ypos();
  newship.land_x = x;
  newship.land_y = y;
  newship.shipclass = (((newship.type == ShipType::OTYPE_TERRA) ||
                        (newship.type == ShipType::OTYPE_PLOW))
                           ? "5"
                           : "Standard");
  newship.whatorbits = ScopeLevel::LEVEL_PLAN;
  newship.whatdest = ScopeLevel::LEVEL_PLAN;
  newship.deststar = snum;
  newship.destpnum = pnum;
  newship.storbits = snum;
  newship.pnumorbits = pnum;
  newship.docked = 1;
  planet.info(Playernum - 1).resource -= newship.build_cost;
  while ((shipno = getdeadship()) == 0)
    ;
  if (shipno == -1) shipno = Numships() + 1;
  newship.number = shipno;
  newship.owner = Playernum;
  newship.governor = Governor;
  newship.ships = 0;
  insert_sh_plan(planet, &newship);
  if (newship.type == ShipType::OTYPE_TOXWC) {
    std::string message = std::format("Toxin concentration on planet was {}%,",
                                      planet.conditions(TOXIC));
    notify(Playernum, Governor, message);
    unsigned char toxic_amount;
    if (planet.conditions(TOXIC) > TOXMAX)
      toxic_amount = TOXMAX;
    else
      toxic_amount = planet.conditions(TOXIC);
    newship.special = WasteData{.toxic = toxic_amount};
    planet.conditions(TOXIC) -= toxic_amount;
    std::string toxMsg = std::format(" now {}%.\n", planet.conditions(TOXIC));
    notify(Playernum, Governor, toxMsg);
  }
  std::string message =
      std::format("{} built at a cost of {} resources.\n",
                  ship_to_string(newship).c_str(), newship.build_cost);
  notify(Playernum, Governor, message);

  std::string techMsg = std::format("Technology {:.1f}.\n", newship.tech);
  notify(Playernum, Governor, techMsg);

  std::string locMsg =
      std::format("{} is on sector {},{}.\n", ship_to_string(newship).c_str(),
                  newship.land_x, newship.land_y);
  notify(Playernum, Governor, locMsg);
}

void create_ship_by_ship(int Playernum, int Governor, const Race& race,
                         bool outside, Planet* planet, Ship* newship,
                         Ship* builder) {
  int shipno;

  while ((shipno = getdeadship()) == 0)
    ;
  if (shipno == -1) shipno = Numships() + 1;
  newship->number = shipno;
  newship->owner = Playernum;
  newship->governor = Governor;
  if (outside) {
    newship->whatorbits = builder->whatorbits;
    newship->whatdest = ScopeLevel::LEVEL_UNIV;
    newship->deststar = builder->deststar;
    newship->destpnum = builder->destpnum;
    newship->storbits = builder->storbits;
    newship->pnumorbits = builder->pnumorbits;
    newship->docked = 0;
    switch (builder->whatorbits) {
      case ScopeLevel::LEVEL_PLAN:
        insert_sh_plan(*planet, newship);
        break;
      case ScopeLevel::LEVEL_STAR:
        insert_sh_star(stars[builder->storbits], newship);
        break;
      case ScopeLevel::LEVEL_UNIV:
        insert_sh_univ(&Sdata, newship);
        break;
      case ScopeLevel::LEVEL_SHIP:
        // TODO(jeffbailey): The compiler can't see that this is impossible.
        break;
    }
  } else {
    newship->whatorbits = ScopeLevel::LEVEL_SHIP;
    newship->whatdest = ScopeLevel::LEVEL_SHIP;
    newship->deststar = builder->deststar;
    newship->destpnum = builder->destpnum;
    newship->destshipno = builder->number;
    newship->storbits = builder->storbits;
    newship->pnumorbits = builder->pnumorbits;
    newship->docked = 1;
    insert_sh_ship(newship, builder);
  }
  newship->tech = race.tech;
  newship->xpos = builder->xpos;
  newship->ypos = builder->ypos;
  newship->land_x = builder->land_x;
  newship->land_y = builder->land_y;
  newship->shipclass = (((newship->type == ShipType::OTYPE_TERRA) ||
                         (newship->type == ShipType::OTYPE_PLOW))
                            ? "5"
                            : "Standard");
  builder->resource -= newship->build_cost;

  std::string message =
      std::format("{} built at a cost of {} resources.\n",
                  ship_to_string(*newship).c_str(), newship->build_cost);
  notify(Playernum, Governor, message);

  std::string techMsg = std::format("Technology {:.1f}.\n", newship->tech);
  notify(Playernum, Governor, techMsg);
}

void Getship(Ship* s, ShipType i, const Race& r) {
  bzero((char*)s, sizeof(Ship));
  s->type = i;
  s->armor = Shipdata[i][ABIL_ARMOR];
  s->guns = Shipdata[i][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE;
  s->primary = Shipdata[i][ABIL_GUNS];
  s->primtype = shipdata_primary(i);
  s->primary = Shipdata[i][ABIL_GUNS];
  s->sectype = shipdata_secondary(i);
  s->max_crew = Shipdata[i][ABIL_MAXCREW];
  s->max_resource = Shipdata[i][ABIL_CARGO];
  s->max_hanger = Shipdata[i][ABIL_HANGER];
  s->max_destruct = Shipdata[i][ABIL_DESTCAP];
  s->max_fuel = Shipdata[i][ABIL_FUELCAP];
  s->max_speed = Shipdata[i][ABIL_SPEED];
  s->build_type = i;
  s->mount = r.God ? Shipdata[i][ABIL_MOUNT] : 0;
  s->hyper_drive.has = r.God ? Shipdata[i][ABIL_JUMP] : 0;
  s->cloak = 0;
  s->laser = r.God ? Shipdata[i][ABIL_LASER] : 0;
  s->cew = 0;
  s->cew_range = 0;
  s->size = ship_size(*s);
  s->base_mass = getmass(*s);
  s->mass = getmass(*s);
  s->build_cost = r.God ? 0 : (int)cost(*s);
  if (s->type == ShipType::OTYPE_VN || s->type == ShipType::OTYPE_BERS) {
    s->special = MindData{.progenitor = static_cast<unsigned char>(r.Playernum),
                          .target = 0,
                          .generation = 0,
                          .busy = 0,
                          .tampered = 0,
                          .who_killed = 0};
  }
}

void Getfactship(Ship* s, Ship* b) {
  bzero((char*)s, sizeof(Ship));
  s->type = b->build_type;
  s->armor = b->armor;
  s->primary = b->primary;
  s->primtype = b->primtype;
  s->secondary = b->secondary;
  s->sectype = b->sectype;
  s->guns = s->primary ? PRIMARY : GTYPE_NONE;
  s->max_crew = b->max_crew;
  s->max_resource = b->max_resource;
  s->max_hanger = b->max_hanger;
  s->max_destruct = b->max_destruct;
  s->max_fuel = b->max_fuel;
  s->max_speed = b->max_speed;
  s->build_type = b->build_type;
  s->build_cost = b->build_cost;
  s->mount = b->mount;
  s->hyper_drive.has = b->hyper_drive.has;
  s->cloak = 0;
  s->laser = b->laser;
  s->cew = b->cew;
  s->cew_range = b->cew_range;
  s->size = ship_size(*s);
  s->base_mass = getmass(*s);
  s->mass = getmass(*s);
}

int Shipcost(ShipType i, const Race& r) {
  Ship s;

  Getship(&s, i, r);
  return ((int)cost(s));
}

std::tuple<money_t, double>
shipping_cost(const starnum_t to, const starnum_t from, const money_t value) {
  double dist = sqrt(Distsq(stars[to].xpos(), stars[to].ypos(),
                            stars[from].xpos(), stars[from].ypos()));

  int junk = (int)(dist / 10000.0);
  junk *= 10000;

  double factor = 1.0 - exp(-(double)junk / MERCHANT_LENGTH);

  money_t fcost = std::round(factor * (double)value);
  return {fcost, dist};
}
