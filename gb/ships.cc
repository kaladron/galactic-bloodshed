// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

// Essentialy everything in this file can move into a Ship class.

/* can takeoff & land, is mobile, etc. */
unsigned short speed_rating(const Ship &s) { return s.max_speed; }

/* has an on/off switch */
bool has_switch(const Ship &s) { return Shipdata[s.type][ABIL_HASSWITCH]; }

/* can bombard planets */
bool can_bombard(const Ship &s) {
  return Shipdata[s.type][ABIL_GUNS] && (s.type != ShipType::STYPE_MINE);
}

/* can navigate */
bool can_navigate(const Ship &s) {
  return Shipdata[s.type][ABIL_SPEED] > 0 && s.type != ShipType::OTYPE_TERRA &&
         s.type != ShipType::OTYPE_VN;
}

/* can aim at things. */
bool can_aim(const Ship &s) {
  return s.type >= ShipType::STYPE_MIRROR && s.type <= ShipType::OTYPE_TRACT;
}

/* macros to get ship stats */
unsigned long armor(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_ARMOR]
                                             : s.armor * (100 - s.damage) / 100;
}

long guns(const Ship &s) {
  return (s.guns == GTYPE_NONE) ? 0
                                : (s.guns == PRIMARY ? s.primary : s.secondary);
}

population_t max_crew(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY)
             ? Shipdata[s.type][ABIL_MAXCREW] - s.troops
             : s.max_crew - s.troops;
}

population_t max_mil(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY)
             ? Shipdata[s.type][ABIL_MAXCREW] - s.popn
             : s.max_crew - s.popn;
}

long max_resource(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_CARGO]
                                             : s.max_resource;
}
int max_crystals(const Ship &) { return MAX_CRYSTALS; }

long max_fuel(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_FUELCAP]
                                             : s.max_fuel;
}

long max_destruct(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_DESTCAP]
                                             : s.max_destruct;
}

long max_speed(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_SPEED]
                                             : s.max_speed;
}

long shipcost(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY)
             ? 2L * s.build_cost * s.on + Shipdata[s.type][ABIL_COST]
             : s.build_cost;
}

double mass(const Ship &s) { return s.mass; }

long shipsight(const Ship &s) {
  return (s.type == ShipType::OTYPE_PROBE) || s.popn;
}

long retaliate(const Ship &s) { return s.retaliate; }

int size(const Ship &s) { return s.size; }

int shipbody(const Ship &s) { return s.size - s.max_hanger; }

long hanger(const Ship &s) { return s.max_hanger - s.hanger; }

long repair(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? s.on : max_crew(s);
}

Shiplist::Iterator::Iterator(shipnum_t a) {
  auto tmpship = getship(a);
  if (tmpship) {
    elem = *tmpship;
  } else {
    elem = Ship{};
    elem.number = 0;
  }
}

Shiplist::Iterator &Shiplist::Iterator::operator++() {
  auto tmpship = getship(elem.nextship);
  if (tmpship) {
    elem = *tmpship;
  } else {
    elem = Ship{};
    elem.number = 0;
  }
  return *this;
}

int getdefense(const Ship &ship) {
  if (landed(ship)) {
    const auto p = getplanet(ship.storbits, ship.pnumorbits);
    const auto sect = getsector(p, ship.land_x, ship.land_y);
    return (2 * Defensedata[sect.condition]);
  }
  // No defense
  return 0;
}

bool laser_on(const Ship &ship) { return (ship.laser && ship.fire_laser); }

bool landed(const Ship &ship) {
  return (ship.whatdest == ScopeLevel::LEVEL_PLAN && ship.docked);
}

void capture_stuff(const Ship &ship, GameObj &g) {
  Shiplist shiplist(ship.ships);
  for (auto s : shiplist) {
    capture_stuff(s, g);  /* recursive call */
    s.owner = ship.owner; /* make sure he gets all of the ships landed on it */
    s.governor = ship.governor;
    putship(&s);
    g.out << ship_to_string(s) << " CAPTURED!\n";
  }
}

std::string ship_to_string(const Ship &s) {
  return std::format("{0}{1} {2} [{3}]", Shipltrs[s.type], s.number, s.name,
                     s.owner);
}

double getmass(const Ship &s) {
  return (1.0 + MASS_ARMOR * s.armor + MASS_SIZE * (s.size - s.max_hanger) +
          MASS_HANGER * s.max_hanger + MASS_GUNS * s.primary * s.primtype +
          MASS_GUNS * s.secondary * s.sectype);
}

unsigned int ship_size(const Ship &s) {
  const double size = 1.0 + SIZE_GUNS * s.primary + SIZE_GUNS * s.secondary +
                      SIZE_CREW * s.max_crew + SIZE_RESOURCE * s.max_resource +
                      SIZE_FUEL * s.max_fuel + SIZE_DESTRUCT * s.max_destruct +
                      s.max_hanger;
  return (std::floor(size));
}

double cost(const Ship &s) {
  /* compute how much it costs to build this ship */
  double factor = 0.0;
  factor += (double)Shipdata[s.build_type][ABIL_COST];
  factor += GUN_COST * (double)s.primary;
  factor += GUN_COST * (double)s.secondary;
  factor += CREW_COST * (double)s.max_crew;
  factor += CARGO_COST * (double)s.max_resource;
  factor += FUEL_COST * (double)s.max_fuel;
  factor += AMMO_COST * (double)s.max_destruct;
  factor +=
      SPEED_COST * (double)s.max_speed * (double)std::sqrt((double)s.max_speed);
  factor += HANGER_COST * (double)s.max_hanger;
  factor += ARMOR_COST * (double)s.armor * (double)std::sqrt((double)s.armor);
  factor += CEW_COST * (double)(s.cew * s.cew_range);
  /* additional advantages/disadvantages */

  double advantage = 0.0;
  advantage += 0.5 * !!s.hyper_drive.has;
  advantage += 0.5 * !!s.laser;
  advantage += 0.5 * !!s.cloak;
  advantage += 0.5 * !!s.mount;

  factor *= std::sqrt(1.0 + advantage);
  return factor;
}

namespace {
/**
 * Permute the system cost advantage and disadvantage based on the given
 * value and base.
 *
 * @param advantage A pointer to a double variable representing the advantage.
 * @param disadvantage A pointer to a double variable representing the
 * disadvantage.
 * @param value An integer value representing the value.
 * @param base An integer value representing the base.
 */
void system_cost(double *advantage, double *disadvantage, int value, int base) {
  const double factor = (((double)value + 1.0) / (base + 1.0)) - 1.0;
  if (factor >= 0.0)
    *advantage += factor;
  else
    *disadvantage -= factor;
}

/* this routine will do landing, launching, loading, unloading, etc
        for merchant ships. The ship is within landing distance of
        the target Planet */
static int do_merchant(Ship &s, Planet &p, std::stringstream &telegram) {
  int i = s.owner - 1;
  int j = s.merchant - 1; /* try to speed things up a bit */

  if (!s.merchant || !p.info[i].route[j].set) /* not on shipping route */
    return 0;
  /* check to see if the sector is owned by the player */
  auto sect = getsector(p, p.info[i].route[j].x, p.info[i].route[j].y);
  if (sect.owner && (sect.owner != s.owner)) {
    return 0;
  }

  if (!landed(s)) { /* try to land the ship */
    double fuel = s.mass * p.gravity() * LAND_GRAV_MASS_FACTOR;
    if (s.fuel < fuel) { /* ship can't land - cancel all orders */
      s.whatdest = ScopeLevel::LEVEL_UNIV;
      telegram << "\t\tNot enough fuel to land!\n";
      return 1;
    }
    s.land_x = p.info[i].route[j].x;
    s.land_y = p.info[i].route[j].y;
    telegram << std::format("\t\tLanded on sector {},{}\n", s.land_x, s.land_y);
    s.xpos = p.xpos + stars[s.storbits].xpos;
    s.ypos = p.ypos + stars[s.storbits].ypos;
    use_fuel(s, fuel);
    s.docked = 1;
    s.whatdest = ScopeLevel::LEVEL_PLAN;
    s.deststar = s.storbits;
    s.destpnum = s.pnumorbits;
  }
  /* load and unload supplies specified by the planet */
  char load = p.info[i].route[j].load;
  char unload = p.info[i].route[j].unload;
  if (load) {
    telegram << "\t\t";
    if (Fuel(load)) {
      int amount = (int)s.max_fuel - (int)s.fuel;
      if (amount > p.info[i].fuel) amount = p.info[i].fuel;
      p.info[i].fuel -= amount;
      rcv_fuel(s, (double)amount);
      telegram << std::format("{}f ", amount);
    }
    if (Resources(load)) {
      int amount = (int)s.max_resource - (int)s.resource;
      if (amount > p.info[i].resource) amount = p.info[i].resource;
      p.info[i].resource -= amount;
      rcv_resource(s, amount);
      telegram << std::format("{}r ", amount);
    }
    if (Crystals(load)) {
      int amount = p.info[i].crystals;
      p.info[i].crystals -= amount;
      s.crystals += amount;
      telegram << std::format("{}x ", amount);
    }
    if (Destruct(load)) {
      int amount = (int)s.max_destruct - (int)s.destruct;
      if (amount > p.info[i].destruct) amount = p.info[i].destruct;
      p.info[i].destruct -= amount;
      rcv_destruct(s, amount);
      telegram << std::format("{}d ", amount);
    }
    telegram << "loaded\n";
  }
  if (unload) {
    telegram << "\t\t";
    if (Fuel(unload)) {
      int amount = (int)s.fuel;
      p.info[i].fuel += amount;
      telegram << std::format("{}f ", amount);
      use_fuel(s, (double)amount);
    }
    if (Resources(unload)) {
      int amount = s.resource;
      p.info[i].resource += amount;
      telegram << std::format("{}r ", amount);
      use_resource(s, amount);
    }
    if (Crystals(unload)) {
      int amount = s.crystals;
      p.info[i].crystals += amount;
      telegram << std::format("{}x ", amount);
      s.crystals -= amount;
    }
    if (Destruct(unload)) {
      int amount = s.destruct;
      p.info[i].destruct += amount;
      telegram << std::format("{}d ", amount);
      use_destruct(s, amount);
    }
    telegram << "unloaded\n";
  }

  /* launch the ship */
  double fuel = s.mass * p.gravity() * LAUNCH_GRAV_MASS_FACTOR;
  if (s.fuel < fuel) {
    telegram << "\t\tNot enough fuel to launch!\n";
    return 1;
  }
  /* ship is ready to fly - order the ship to its next destination */
  s.whatdest = ScopeLevel::LEVEL_PLAN;
  s.deststar = p.info[i].route[j].dest_star;
  s.destpnum = p.info[i].route[j].dest_planet;
  s.docked = 0;
  use_fuel(s, fuel);
  telegram << std::format("\t\tDestination set to {}\n", prin_ship_dest(s));
  if (s.hyper_drive.has) { /* order the ship to jump if it can */
    if (s.storbits != s.deststar) {
      s.navigate.on = 0;
      s.hyper_drive.on = 1;
      if (s.mounted) {
        s.hyper_drive.charge = 1;
        s.hyper_drive.ready = 1;
      } else {
        s.hyper_drive.charge = 0;
        s.hyper_drive.ready = 0;
      }
      telegram << "\t\tJump orders set\n";
    }
  }
  return 1;
}

}  // namespace

/**
 * Calculates the complexity of a ship.
 *
 * The complexity of a ship is used in two ways:
 * 1) Determine whether a ship is too complex for the race to create, limited
 * by race.tech.
 * 2) Used in sorting the order of the ships for display.
 *
 * This is a custom algorithm to GB.
 *
 * @param s The Ship object for which the complexity is calculated.
 * @return The complexity value of the ship.
 */
double complexity(const Ship &s) {
  double advantage = 0;
  double disadvantage = 0;

  system_cost(&advantage, &disadvantage, s.primary,
              Shipdata[s.build_type][ABIL_GUNS]);
  system_cost(
      &advantage, &disadvantage, s.secondary, Shipdata[s.build_type][ABIL_GUNS]

  );
  system_cost(&advantage, &disadvantage, s.max_crew,
              Shipdata[s.build_type][ABIL_MAXCREW]);
  system_cost(&advantage, &disadvantage, s.max_resource,
              Shipdata[s.build_type][ABIL_CARGO]);
  system_cost(&advantage, &disadvantage, s.max_fuel,
              Shipdata[s.build_type][ABIL_FUELCAP]);
  system_cost(&advantage, &disadvantage, s.max_destruct,
              Shipdata[s.build_type][ABIL_DESTCAP]);
  system_cost(&advantage, &disadvantage, s.max_speed,
              Shipdata[s.build_type][ABIL_SPEED]);
  system_cost(&advantage, &disadvantage, s.max_hanger,
              Shipdata[s.build_type][ABIL_HANGER]);
  system_cost(&advantage, &disadvantage, s.armor,
              Shipdata[s.build_type][ABIL_ARMOR]);

  // additional advantages/disadvantages
  // TODO(jeffbailey): document this function in English.
  double factor =
      std::sqrt((1.0 + advantage) * std::exp(-(double)disadvantage / 10.0));
  const double tmp =
      COMPLEXITY_FACTOR * (factor - 1.0) /
          std::sqrt((double)(Shipdata[s.build_type][ABIL_TECH] + 1)) +
      1.0;
  factor = tmp * tmp;
  return (factor * (double)Shipdata[s.build_type][ABIL_TECH]);
}

bool testship(const Ship &s, GameObj &g) {
  const player_t playernum = g.player;
  const governor_t governor = g.governor;
  if (!s.alive) {
    g.out << std::format("{} has been destroyed.\n", ship_to_string(s));
    return true;
  }

  if (s.owner != playernum || !authorized(governor, s)) {
    DontOwnErr(playernum, governor, s.number);
    return true;
  }

  if (!s.active) {
    g.out << std::format("{} is irradiated {}% and inactive.\n",
                         ship_to_string(s), s.rad);
    return true;
  }

  return false;
}

void kill_ship(player_t Playernum, Ship *ship) {
  ship->special.mind.who_killed = Playernum;
  ship->alive = 0;
  ship->notified = 0; /* prepare the ship for recycling */

  if (ship->type != ShipType::STYPE_POD &&
      ship->type != ShipType::OTYPE_FACTORY) {
    /* pods don't do things to morale, ditto for factories */
    auto &victim = races[ship->owner - 1];
    if (victim.Gov_ship == ship->number) victim.Gov_ship = 0;
    if (!victim.God && Playernum != ship->owner &&
        ship->type != ShipType::OTYPE_VN) {
      auto &killer = races[Playernum - 1];
      adjust_morale(killer, victim, (int)ship->build_cost);
      putrace(killer);
    } else if (ship->owner == Playernum && !ship->docked && max_crew(*ship)) {
      victim.morale -= 2L * ship->build_cost; /* scuttle/scrap */
    }
    putrace(victim);
  }

  if (ship->type == ShipType::OTYPE_VN || ship->type == ShipType::OTYPE_BERS) {
    getsdata(&Sdata);
    /* add ship to VN shit list */
    Sdata.VN_hitlist[ship->special.mind.who_killed - 1] += 1;

    /* keep track of where these VN's were shot up */

    if (Sdata.VN_index1[Playernum - 1] == -1)
      /* there's no star in the first index */
      Sdata.VN_index1[Playernum - 1] = ship->storbits;
    else if (Sdata.VN_index2[Playernum - 1] == -1)
      /* there's no star in the second index */
      Sdata.VN_index2[Playernum - 1] = ship->storbits;
    else {
      /* pick an index to supplant */
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<int> dis(0, 1);
      if (dis(gen))
        Sdata.VN_index1[Playernum - 1] = ship->storbits;
      else
        Sdata.VN_index2[Playernum - 1] = ship->storbits;
    }
    putsdata(&Sdata);
  }

  if (ship->type == ShipType::OTYPE_TOXWC &&
      ship->whatorbits == ScopeLevel::LEVEL_PLAN) {
    auto planet = getplanet(ship->storbits, ship->pnumorbits);
    planet.conditions[TOXIC] =
        MIN(100, planet.conditions[TOXIC] + ship->special.waste.toxic);
    putplanet(planet, stars[ship->storbits], ship->pnumorbits);
  }

  /* undock the stuff docked with it */
  if (ship->docked && ship->whatorbits != ScopeLevel::LEVEL_SHIP &&
      ship->whatdest == ScopeLevel::LEVEL_SHIP) {
    auto s = getship(ship->destshipno);
    if (!s) {
      std::cerr << "Database corruption, ship not found.";
      std::abort();
    }
    s->docked = 0;
    s->whatdest = ScopeLevel::LEVEL_UNIV;
    putship(&*s);
  }
  /* landed ships are killed */
  Shiplist shiplist(ship->ships);
  for (auto s : shiplist) {
    kill_ship(Playernum, &s);
    putship(&s);
  }
}

std::string dispshiploc_brief(const Ship &ship) {
  switch (ship.whatorbits) {
    case ScopeLevel::LEVEL_STAR:
      return std::format("/{0:4.4s}", stars[ship.storbits].name);
    case ScopeLevel::LEVEL_PLAN:
      return std::format("/{0}/{1:4.4s}", stars[ship.storbits].name,
                         stars[ship.storbits].pnames[ship.pnumorbits]);
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{0}", ship.destshipno);
    case ScopeLevel::LEVEL_UNIV:
      return "/";
  }
}

std::string dispshiploc(const Ship &ship) {
  switch (ship.whatorbits) {
    case ScopeLevel::LEVEL_STAR:
      return std::format("/{0}", stars[ship.storbits].name);
    case ScopeLevel::LEVEL_PLAN:
      return std::format("/{0}/{1}", stars[ship.storbits].name,
                         stars[ship.storbits].pnames[ship.pnumorbits]);
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{0}", ship.destshipno);
    case ScopeLevel::LEVEL_UNIV:
      return "/";
  }
}

/// Determine whether the ship crashed or not.
std::tuple<bool, int> crash(const Ship &s, const double fuel) noexcept {
  // Crash from insufficient fuel.
  if (s.fuel < fuel) return {true, 0};

  // Damaged ships stand of chance of crash landing.
  if (auto roll = int_rand(1, 100); roll <= s.damage) return {true, roll};

  // No crash.
  return {false, 0};
}

int docked(const Ship &s) {
  return s.docked && s.whatdest == ScopeLevel::LEVEL_SHIP;
}

int overloaded(const Ship &s) {
  return (s.resource > max_resource(s)) || (s.fuel > max_fuel(s)) ||
         (s.popn + s.troops > s.max_crew) || (s.destruct > max_destruct(s));
}

std::string prin_ship_orbits(const Ship &s) {
  switch (s.whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      return std::format("/({:0.0},{:1.0})", s.xpos, s.ypos);
    case ScopeLevel::LEVEL_STAR:
      return std::format("/{0}", stars[s.storbits].name);
    case ScopeLevel::LEVEL_PLAN:
      return std::format("/{0}/{1}", stars[s.storbits].name,
                         stars[s.storbits].pnames[s.pnumorbits]);
    case ScopeLevel::LEVEL_SHIP:
      if (auto mothership = getship(s.destshipno); mothership) {
        return prin_ship_orbits(*mothership);
      } else {
        return "/";
      }
  }
}

std::string prin_ship_dest(const Ship &ship) {
  Place dest{ship.whatdest, ship.deststar, ship.destpnum, ship.destshipno};
  return dest.to_string();
}

void moveship(Ship &s, int mode, int send_messages, int checking_fuel) {
  double stardist;
  double movedist;
  double truedist;
  double dist;
  double xdest;
  double ydest;
  double sn;
  double cs;
  double mfactor;
  double heading;
  double distfac;
  double fuse;
  ScopeLevel destlevel;
  int deststar = 0;
  int destpnum = 0;
  Ship *dsh;

  if (s.hyper_drive.has && s.hyper_drive.on) { /* do a hyperspace jump */
    if (!mode) return; /* we're not ready to jump until the update */
    if (s.hyper_drive.ready) {
      dist = std::sqrt(Distsq(s.xpos, s.ypos, stars[s.deststar].xpos,
                              stars[s.deststar].ypos));
      distfac = HYPER_DIST_FACTOR * (s.tech + 100.0);
      if (s.mounted && dist > distfac)
        fuse = HYPER_DRIVE_FUEL_USE * std::sqrt(s.mass) * (dist / distfac);
      else
        fuse = HYPER_DRIVE_FUEL_USE * std::sqrt(s.mass) * (dist / distfac) *
               (dist / distfac);

      if (s.fuel < fuse) {
        std::string telegram = std::format(
            "{} at system {} does not have {:.1f}f to do hyperspace jump.",
            ship_to_string(s), prin_ship_orbits(s), fuse);
        if (send_messages) push_telegram(s.owner, s.governor, telegram);
        s.hyper_drive.on = 0;
        return;
      }
      use_fuel(s, fuse);
      heading = std::atan2(stars[s.deststar].xpos - s.xpos,
                           stars[s.deststar].ypos - s.ypos);
      sn = std::sin(heading);
      cs = std::cos(heading);
      s.xpos = stars[s.deststar].xpos - sn * 0.9 * SYSTEMSIZE;
      s.ypos = stars[s.deststar].ypos - cs * 0.9 * SYSTEMSIZE;
      s.whatorbits = ScopeLevel::LEVEL_STAR;
      s.storbits = s.deststar;
      s.protect.planet = 0;
      s.hyper_drive.on = 0;
      s.hyper_drive.ready = 0;
      s.hyper_drive.charge = 0;
      std::string telegram = std::format("{} arrived at {}.", ship_to_string(s),
                                         prin_ship_orbits(s));
      if (send_messages) push_telegram(s.owner, s.governor, telegram);
    } else if (s.mounted) {
      s.hyper_drive.ready = 1;
      s.hyper_drive.charge = HYPER_DRIVE_READY_CHARGE;
    } else {
      if (s.hyper_drive.charge == HYPER_DRIVE_READY_CHARGE)
        s.hyper_drive.ready = 1;
      else
        s.hyper_drive.charge += 1;
    }
    return;
  }
  if (s.speed && !s.docked && s.alive &&
      (s.whatdest != ScopeLevel::LEVEL_UNIV || s.navigate.on)) {
    fuse = 0.5 * s.speed * (1 + s.protect.evade) * s.mass * FUEL_USE /
           (double)segments;
    if (s.fuel < fuse) {
      if (send_messages) msg_OOF(s); /* send OOF notify */
      if (s.whatorbits == ScopeLevel::LEVEL_UNIV &&
          (s.build_cost <= 50 || s.type == ShipType::OTYPE_VN ||
           s.type == ShipType::OTYPE_BERS)) {
        std::string telegram =
            std::format("{} has been lost in deep space.", ship_to_string(s));
        if (send_messages) push_telegram(s.owner, s.governor, telegram);
        if (send_messages) kill_ship((int)(s.owner), &s);
      }
      return;
    }
    if (s.navigate.on) { /* follow navigational orders */
      heading = .0174329252 * s.navigate.bearing;
      mfactor = SHIP_MOVE_SCALE * (1.0 - .01 * s.rad) * (1.0 - .01 * s.damage) *
                SpeedConsts[s.speed] * MoveConsts[s.whatorbits] /
                (double)segments;
      use_fuel(s, (double)fuse);
      sn = std::sin(heading);
      cs = std::cos(heading);
      xdest = sn * mfactor;
      ydest = -cs * mfactor;
      s.xpos += xdest;
      s.ypos += ydest;
      s.navigate.turns--;
      if (!s.navigate.turns) s.navigate.on = 0;
      /* check here for orbit breaking as well. Maarten */
      auto &ost = stars[s.storbits];
      const auto &opl = planets[s.storbits][s.pnumorbits];
      if (s.whatorbits == ScopeLevel::LEVEL_PLAN) {
        dist = std::sqrt(
            Distsq(s.xpos, s.ypos, ost.xpos + opl->xpos, ost.ypos + opl->ypos));
        if (dist > PLORBITSIZE) {
          s.whatorbits = ScopeLevel::LEVEL_STAR;
          s.protect.planet = 0;
        }
      } else if (s.whatorbits == ScopeLevel::LEVEL_STAR) {
        dist = std::sqrt(Distsq(s.xpos, s.ypos, ost.xpos, ost.ypos));
        if (dist > SYSTEMSIZE) {
          s.whatorbits = ScopeLevel::LEVEL_UNIV;
          s.protect.evade = 0;
          s.protect.planet = 0;
        }
      }
    } else { /*		navigate is off            */
      destlevel = s.whatdest;
      if (destlevel == ScopeLevel::LEVEL_SHIP) {
        dsh = ships[s.destshipno];
        s.deststar = dsh->storbits;
        s.destpnum = dsh->pnumorbits;
        xdest = dsh->xpos;
        ydest = dsh->ypos;
        switch (dsh->whatorbits) {
          case ScopeLevel::LEVEL_UNIV:
            break;
          case ScopeLevel::LEVEL_PLAN:
            if (s.whatorbits != dsh->whatorbits ||
                s.pnumorbits != dsh->pnumorbits)
              destlevel = ScopeLevel::LEVEL_PLAN;
            break;
          case ScopeLevel::LEVEL_STAR:
            if (s.whatorbits != dsh->whatorbits || s.storbits != dsh->storbits)
              destlevel = ScopeLevel::LEVEL_STAR;
            break;
          case ScopeLevel::LEVEL_SHIP:
            // TODO(jeffbailey): Prove that this is impossible.
            break;
        }
        /*			if (std::sqrt( (double)Distsq(s.xpos, s.ypos,
           xdest,
           ydest))
                   <= DIST_TO_LAND || !(dsh->alive)) {
                           destlevel = ScopeLevel::LEVEL_UNIV;
                                                   s.whatdest=ScopeLevel::LEVEL_UNIV;
                                   } */
      }
      /*		else */
      if (destlevel == ScopeLevel::LEVEL_STAR ||
          (destlevel == ScopeLevel::LEVEL_PLAN &&
           (s.storbits != s.deststar ||
            s.whatorbits == ScopeLevel::LEVEL_UNIV))) {
        destlevel = ScopeLevel::LEVEL_STAR;
        deststar = s.deststar;
        xdest = stars[deststar].xpos;
        ydest = stars[deststar].ypos;
      } else if (destlevel == ScopeLevel::LEVEL_PLAN &&
                 s.storbits == s.deststar) {
        destlevel = ScopeLevel::LEVEL_PLAN;
        deststar = s.deststar;
        destpnum = s.destpnum;
        xdest = stars[deststar].xpos + planets[deststar][destpnum]->xpos;
        ydest = stars[deststar].ypos + planets[deststar][destpnum]->ypos;
        if (std::sqrt(Distsq(s.xpos, s.ypos, xdest, ydest)) <= DIST_TO_LAND)
          destlevel = ScopeLevel::LEVEL_UNIV;
      }
      auto &dst = stars[deststar];
      auto &ost = stars[s.storbits];
      const auto &dpl = planets[deststar][destpnum];
      const auto &opl = planets[s.storbits][s.pnumorbits];
      truedist = movedist = std::sqrt(Distsq(s.xpos, s.ypos, xdest, ydest));
      /* Save some unneccesary calculation and domain errors for atan2
            Maarten */
      if (truedist < DIST_TO_LAND && s.whatorbits == destlevel &&
          s.storbits == deststar && s.pnumorbits == destpnum)
        return;
      heading = std::atan2((double)(xdest - s.xpos), (double)(-ydest + s.ypos));
      mfactor = SHIP_MOVE_SCALE * (1. - .01 * (double)s.rad) *
                (1. - .01 * (double)s.damage) * SpeedConsts[s.speed] *
                MoveConsts[s.whatorbits] / (double)segments;

      /* keep from ending up in the middle of the system. */
      if (destlevel == ScopeLevel::LEVEL_STAR &&
          (s.storbits != deststar || s.whatorbits == ScopeLevel::LEVEL_UNIV))
        movedist -= SYSTEMSIZE * 0.90;
      else if (destlevel == ScopeLevel::LEVEL_PLAN &&
               s.whatorbits == ScopeLevel::LEVEL_STAR &&
               s.storbits == deststar && truedist >= PLORBITSIZE)
        movedist -= PLORBITSIZE * 0.90;

      if (s.whatdest == ScopeLevel::LEVEL_SHIP &&
          !followable(s, *ships[s.destshipno])) {
        s.whatdest = ScopeLevel::LEVEL_UNIV;
        s.protect.evade = 0;
        std::string telegram =
            std::format("{} at {} lost sight of destination ship #{}.",
                        ship_to_string(s), prin_ship_orbits(s), s.destshipno);
        if (send_messages) push_telegram(s.owner, s.governor, telegram);
        return;
      }
      if (truedist > DIST_TO_LAND) {
        use_fuel(s, (double)fuse);
        /* dont overshoot */
        sn = std::sin(heading);
        cs = std::cos(heading);
        xdest = sn * mfactor;
        ydest = -cs * mfactor;
        if (std::hypot(xdest, ydest) > movedist) {
          xdest = sn * movedist;
          ydest = -cs * movedist;
        }
        s.xpos += xdest;
        s.ypos += ydest;
      }
      /***** check if far enough away from object it's orbiting to break orbit
       * *****/
      if (s.whatorbits == ScopeLevel::LEVEL_PLAN) {
        dist = std::sqrt(
            Distsq(s.xpos, s.ypos, ost.xpos + opl->xpos, ost.ypos + opl->ypos));
        if (dist > PLORBITSIZE) {
          s.whatorbits = ScopeLevel::LEVEL_STAR;
          s.protect.planet = 0;
        }
      } else if (s.whatorbits == ScopeLevel::LEVEL_STAR) {
        dist = std::sqrt(Distsq(s.xpos, s.ypos, ost.xpos, ost.ypos));
        if (dist > SYSTEMSIZE) {
          s.whatorbits = ScopeLevel::LEVEL_UNIV;
          s.protect.evade = 0;
          s.protect.planet = 0;
        }
      }

      /*******   check for arriving at destination *******/
      if (destlevel == ScopeLevel::LEVEL_STAR ||
          (destlevel == ScopeLevel::LEVEL_PLAN &&
           (s.storbits != deststar ||
            s.whatorbits == ScopeLevel::LEVEL_UNIV))) {
        stardist = std::sqrt(Distsq(s.xpos, s.ypos, dst.xpos, dst.ypos));
        if (stardist <= SYSTEMSIZE * 1.5) {
          s.whatorbits = ScopeLevel::LEVEL_STAR;
          s.protect.planet = 0;
          s.storbits = deststar;
          /* if this system isn't inhabited by you, give it to the
             governor of the ship */
          if (!checking_fuel && (s.popn || s.type == ShipType::OTYPE_PROBE)) {
            if (!isset(dst.inhabited, s.owner))
              dst.governor[s.owner - 1] = s.governor;
            setbit(dst.explored, s.owner);
            setbit(dst.inhabited, s.owner);
          }
          if (s.type != ShipType::OTYPE_VN) {
            std::string telegram = std::format(
                "{} arrived at {}.", ship_to_string(s), prin_ship_orbits(s));
            if (send_messages) push_telegram(s.owner, s.governor, telegram);
          }
          if (s.whatdest == ScopeLevel::LEVEL_STAR)
            s.whatdest = ScopeLevel::LEVEL_UNIV;
        }
      } else if (destlevel == ScopeLevel::LEVEL_PLAN &&
                 deststar == s.storbits) {
        /* headed for a planet in the same system, & not already there.. */
        dist = std::sqrt(
            Distsq(s.xpos, s.ypos, dst.xpos + dpl->xpos, dst.ypos + dpl->ypos));
        if (dist <= PLORBITSIZE) {
          if (!checking_fuel && (s.popn || s.type == ShipType::OTYPE_PROBE)) {
            dpl->info[s.owner - 1].explored = 1;
            setbit(dst.explored, s.owner);
            setbit(dst.inhabited, s.owner);
          }
          s.whatorbits = ScopeLevel::LEVEL_PLAN;
          s.pnumorbits = destpnum;
          std::stringstream telegram;
          if (dist <= (double)DIST_TO_LAND) {
            telegram << std::format("{} within landing distance of {}.",
                                    ship_to_string(s), prin_ship_orbits(s));
            if (checking_fuel || !do_merchant(s, *dpl, telegram))
              if (s.whatdest == ScopeLevel::LEVEL_PLAN)
                s.whatdest = ScopeLevel::LEVEL_UNIV;
          } else {
            telegram << std::format("{} arriving at {}.", ship_to_string(s),
                                    prin_ship_orbits(s));
          }
          if (s.type == ShipType::STYPE_OAP) {
            telegram << std::format(
                "\nEnslavement of the planet is now possible.");
          }
          if (send_messages && s.type != ShipType::OTYPE_VN)
            push_telegram(s.owner, s.governor, telegram.str());
        }
      } else if (destlevel == ScopeLevel::LEVEL_SHIP) {
        dist = std::sqrt(Distsq(s.xpos, s.ypos, dsh->xpos, dsh->ypos));
        if (dist <= PLORBITSIZE) {
          if (dsh->whatorbits == ScopeLevel::LEVEL_PLAN) {
            s.whatorbits = ScopeLevel::LEVEL_PLAN;
            s.storbits = dsh->storbits;
            s.pnumorbits = dsh->pnumorbits;
          } else if (dsh->whatorbits == ScopeLevel::LEVEL_STAR) {
            s.whatorbits = ScopeLevel::LEVEL_STAR;
            s.storbits = dsh->storbits;
            s.protect.planet = 0;
          }
        }
      }
    } /* 'destination' orders */
  } /* if impulse drive */
}  // namespace void moveship(Ship&s,intmode,intsend_messages,intchecking_fuel)

/* deliver an "out of fuel" message.  Used by a number of ship-updating
 *  code segments; so that code isn't duplicated.
 */
void msg_OOF(const Ship &s) {
  std::string telegram = std::format("{} is out of fuel at {}.",
                                     ship_to_string(s), prin_ship_orbits(s));
  push_telegram(s.owner, s.governor, telegram);
}

/* followable: returns 1 iff s1 can follow s2 */
bool followable(const Ship &s1, Ship &s2) {
  if (!s2.alive || !s1.active || s2.whatorbits == ScopeLevel::LEVEL_SHIP)
    return true;

  double dx = s1.xpos - s2.xpos;
  double dy = s1.ypos - s2.ypos;

  double range = 4.0 * logscale((int)(s1.tech + 1.0)) * SYSTEMSIZE;

  auto &r = races[s2.owner - 1];
  auto allied = r.allied;
  /* You can follow your own ships, your allies' ships, or nearby ships */
  return (s1.owner == s2.owner) || (isset(allied, s1.owner)) ||
         (std::sqrt(dx * dx + dy * dy) <= range);
}
