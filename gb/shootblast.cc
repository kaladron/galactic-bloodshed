// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

module gblib;

static std::pair<int, std::string> do_radiation(Ship &ship, double tech,
                                                int strength, int hits);
static std::pair<int, std::string> do_damage(player_t who, Ship &ship,
                                             double tech, int strength,
                                             int hits, int defense, int caliber,
                                             double range,
                                             const std::string_view weapon,
                                             int hit_probability);

static std::tuple<int, int, int> ship_disposition(const Ship &ship);
static int CEW_hit(double dist, int cew_range);
static int Num_hits(double dist, bool focus, int strength, double tech,
                    int damage, int fevade, int tevade, int fspeed, int tspeed,
                    int tbody, guntype_t caliber, int defense,
                    int *hit_probability);
static int cew_hit_odds(double dist, int cew_range);
static std::string do_critical_hits(int penetrate, Ship &ship, int *hits,
                                    int *damage, int defense);
static double p_factor(double attacker, double defender);
static void mutate_sector(Sector &s);

std::optional<std::tuple<int, std::string, std::string>> shoot_ship_to_ship(
    const Ship &attacker, Ship &target, const int cew_strength, const int range,
    const bool ignore) {
  if (cew_strength <= 0) return std::nullopt;

  if (!(attacker.alive || ignore) || !target.alive) return std::nullopt;
  if (attacker.whatorbits == ScopeLevel::LEVEL_SHIP ||
      target.whatorbits == ScopeLevel::LEVEL_UNIV)
    return std::nullopt;
  if (target.whatorbits == ScopeLevel::LEVEL_SHIP ||
      target.whatorbits == ScopeLevel::LEVEL_UNIV)
    return std::nullopt;
  if (attacker.storbits != target.storbits) return std::nullopt;
  if (has_switch(attacker) && !attacker.on) return std::nullopt;

  /* compute caliber */
  const auto caliber = current_caliber(attacker);

  double dist = [&attacker, &target]() -> double {
    if (attacker.type ==
        ShipType::STYPE_MISSILE) /* missiles hit at point blank range */
      return 0.0;

    double dist = std::sqrt(
        Distsq(attacker.xpos, attacker.ypos, target.xpos, target.ypos));
    if (attacker.type ==
        ShipType::STYPE_MINE) { /* compute the effective range */
      dist *= dist / 200.0;     /* mines are very effective inside 200 */
    }
    return dist;
  }();

  if (dist > gun_range(attacker)) return std::nullopt;

  /* attack parameters */
  auto [fevade, fspeed, fbody] = ship_disposition(attacker);
  auto [tevade, tspeed, tbody] = ship_disposition(target);
  auto defense = getdefense(target);

  bool focus = laser_on(attacker) && attacker.focus;

  int hit_probability;
  int hits = (range != 0)
                 ? cew_strength * CEW_hit(dist, (int)attacker.cew_range)
                 : Num_hits(dist, focus, cew_strength, attacker.tech,
                            (int)attacker.damage, fevade, tevade, fspeed,
                            tspeed, tbody, caliber, defense, &hit_probability);

  // mode is whether a ship has been set to radiative with the orders command.
  if (attacker.mode) {
    auto [damage, damage_msg] =
        do_radiation(target, attacker.tech, cew_strength, hits);
    std::string short_msg = std::format(
        "{}: {} {} {}\n", dispshiploc(target), ship_to_string(attacker),
        target.alive ? "attacked" : "DESTROYED", ship_to_string(target));
    std::string long_msg = short_msg;
    long_msg += damage_msg;
    return std::make_tuple(damage, short_msg, long_msg);
    ;
  }

  // CEW, destruct, lasers
  auto weapon = [range, attacker, caliber] -> std::string {
    if (range != 0) return "strength CEW";

    if (laser_on(attacker)) {
      if (attacker.focus) return "strength focused laser";
      return "strength laser";
    }

    switch (caliber) {
      case GTYPE_LIGHT:
        return "light guns";
      case GTYPE_MEDIUM:
        return "medium guns";
      case GTYPE_HEAVY:
        return "heavy guns";
      case GTYPE_NONE:
        return "pea-shooter";
    }
  }();

  if (caliber == GTYPE_NONE) return std::nullopt;

  auto [damage, damage_msg] =
      do_damage(attacker.owner, target, (double)attacker.tech, cew_strength,
                hits, defense, caliber, dist, weapon, hit_probability);
  std::string short_msg = std::format(
      "{}: {} {} {}\n", dispshiploc(target), ship_to_string(attacker),
      target.alive ? "attacked" : "DESTROYED", ship_to_string(target));
  std::string long_msg = short_msg;
  long_msg += damage_msg;
  return std::make_tuple(damage, short_msg, long_msg);
}

int shoot_planet_to_ship(Race &race, Ship &ship, int strength, char *long_msg,
                         char *short_msg) {
  if (strength <= 0) return -1;
  if (!ship.alive) return -1;

  if (ship.whatorbits != ScopeLevel::LEVEL_PLAN) return -1;

  auto [evade, speed, body] = ship_disposition(ship);

  int hit_probability;
  int hits = Num_hits(0.0, false, strength, race.tech, 0, evade, 0, speed, 0,
                      body, GTYPE_MEDIUM, 1, &hit_probability);

  auto [damage, damage_msg] =
      do_damage(race.Playernum, ship, race.tech, strength, hits, 0,
                GTYPE_MEDIUM, 0.0, "medium guns", hit_probability);
  sprintf(short_msg, "%s [%d] %s %s\n", dispshiploc(ship).c_str(),
          race.Playernum, ship.alive ? "attacked" : "DESTROYED",
          ship_to_string(ship).c_str());
  strcpy(long_msg, short_msg);
  strcat(long_msg, damage_msg.c_str());

  return damage;
}

/**
 * @return Number of sectors destroyed.
 */
int shoot_ship_to_planet(Ship &ship, Planet &pl, int strength, int x, int y,
                         SectorMap &smap, int ignore, int caliber,
                         char *long_msg, char *short_msg) {
  int numdest = 0;

  if (strength <= 0) return -1;
  if (!(ship.alive || ignore)) return -1;
  if (has_switch(ship) && !ship.on) return -1;
  if (ship.whatorbits != ScopeLevel::LEVEL_PLAN) return -1;

  if (x < 0 || x > pl.Maxx() - 1 || y < 0 || y > pl.Maxy() - 1) return -1;

  double r = .4 * strength;
  if (!caliber) { /* figure out the appropriate gun caliber if not given*/
    if (ship.fire_laser)
      caliber = GTYPE_LIGHT;
    else
      switch (ship.guns) {
        case PRIMARY:
          caliber = ship.primtype;
          break;
        case SECONDARY:
          caliber = ship.sectype;
          break;
        default:
          caliber = GTYPE_LIGHT;
      }
  }

  auto &target = smap.get(x, y);
  player_t oldowner = target.owner;

  std::array<int, MAXPLAYERS> sum_mob{0};

  for (auto y2 = 0; y2 < pl.Maxy(); y2++) {
    for (auto x2 = 0; x2 < pl.Maxx(); x2++) {
      int dx = std::min(std::abs(x2 - x), std::abs(x + (pl.Maxx() - 1) - x2));
      int dy = std::abs(y2 - y);
      double d = std::sqrt((double)(dx * dx + dy * dy));
      auto &s = smap.get(x2, y2);

      if (d <= r) {
        double fac =
            SECTOR_DAMAGE * (double)strength * (double)caliber / (d + 1.);

        if (s.owner) {
          population_t kills = 0;
          if (s.popn) {
            kills = int_rand(0, ((int)(fac / 10.0) * s.popn)) /
                    (1 + (s.condition == SectorType::SEC_PLATED));
            if (kills > s.popn)
              s.popn = 0;
            else
              s.popn -= kills;
          }
          if (s.troops && (fac > 5.0 * (double)Defensedata[s.condition])) {
            kills = int_rand(0, ((int)(fac / 20.0) * s.troops)) /
                    (1 + (s.condition == SectorType::SEC_PLATED));
            if (kills > s.troops)
              s.troops = 0;
            else
              s.troops -= kills;
          }

          if (!(s.popn + s.troops)) s.owner = 0;
        }

        if (fac >= 5.0 && !int_rand(0, 10)) mutate_sector(s);

        if (round_rand(fac) > Defensedata[s.condition] * int_rand(0, 10)) {
          if (s.owner) Nuked[s.owner - 1] = 1;
          s.popn = 0;
          s.troops = int_rand(0, (int)s.troops);
          if (!s.troops) /* troops may survive this */
            s.owner = 0;
          s.eff = 0;
          s.resource = s.resource / ((int)fac + 1);
          s.mobilization = 0;
          s.fert = 0; /*all is lost !*/
          s.crystals = int_rand(0, (int)s.crystals);
          s.condition = SectorType::SEC_WASTED;
          numdest++;
        } else {
          s.fert = std::max(0, (int)s.fert - (int)fac);
          s.eff = std::max(0, (int)s.eff - (int)fac);
          s.mobilization = std::max(0, (int)s.mobilization - (int)fac);
          s.resource = std::max(0, (int)s.resource - (int)fac);
        }
      }
      if (s.owner) sum_mob[s.owner - 1] += s.mobilization;
    }
  }
  auto num_sectors = pl.Maxx() * pl.Maxy();
  for (auto i = 1; i <= Num_races; i++) {
    pl.info(i - 1).mob_points = sum_mob[i - 1];
    pl.info(i - 1).comread = sum_mob[i - 1] / num_sectors;
    pl.info(i - 1).guns = planet_guns(sum_mob[i - 1]);
  }

  /* planet toxicity goes up a bit */
  pl.conditions(TOXIC) += (100 - pl.conditions(TOXIC)) *
                          ((double)numdest / (double)(pl.Maxx() * pl.Maxy()));

  sprintf(short_msg, "%s bombards %s [%d]\n", ship_to_string(ship).c_str(),
          dispshiploc(ship).c_str(), oldowner);
  strcpy(long_msg, short_msg);
  std::string msg = std::format("\t{} sectors destroyed\n", numdest);
  strcat(long_msg, msg.c_str());
  return numdest;
}

static std::pair<int, std::string> do_radiation(Ship &ship, double tech,
                                                int strength, int hits) {
  std::stringstream msg;
  double fac = (2. / 3.14159265) *
               std::atan((double)(5 * (tech + 1.0) / (ship.tech + 1.0)));

  int arm = std::max(0UL, armor(ship) - hits / 5);
  int body = shipbody(ship);

  int penetrate = 0;
  double r = 1.0;
  for (int i = 1; i <= arm; i++) r *= fac;

  for (int i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r) penetrate += 1;

  int dosage = round_rand(40. * (double)penetrate / (double)body);
  dosage = std::min(100, dosage);

  if (dosage > ship.rad) ship.rad = std::max(ship.rad, dosage);
  if (success(ship.rad)) ship.active = 0;

  int casualties = 0;
  int casualties1 = 0;
  msg << std::format("\tAttack: {} radiation\n\t  Hits: {}\n", strength, hits);
  msg << std::format("\t   Rad: {}% for a total of {}%\n", dosage, ship.rad);
  if (casualties || casualties1) {
    msg << std::format("\tKilled: {} civ + {} mil\n", casualties, casualties1);
  }
  return {dosage, msg.str()};
}

static std::pair<int, std::string> do_damage(player_t who, Ship &ship,
                                             double tech, int strength,
                                             int hits, int defense, int caliber,
                                             double range,
                                             const std::string_view weapon,
                                             int hit_probability) {
  std::stringstream msg;

  msg << std::format("\tAttack: {} {} at a range of {:.0f}\n", strength, weapon,
                     range);
  msg << std::format("\t  Hits: {}  {}% probability\n", hits, hit_probability);
  /* ship may lose some armor */
  if (ship.armor)
    if (success(hits * caliber)) {
      ship.armor--;
      msg << std::format("\t\tArmor reduced to {}\n", ship.armor);
    }

  double fac = p_factor(tech, ship.tech);
  int arm = std::max(0UL, armor(ship) + defense - hits / 5);
  double body = std::sqrt((double)(0.1 * shipbody(ship)));

  int critdam = 0;
  int crithits = 0;
  int penetrate = 0;
  double r = 1.0;
  for (int i = 1; i <= arm; i++) r *= fac;

  for (int i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r) penetrate += 1;

  int damage = round_rand(SHIP_DAMAGE * (double)caliber * (double)penetrate /
                          (double)body);

  auto critmsg =
      do_critical_hits(penetrate, ship, &crithits, &critdam, caliber);

  if (crithits) damage += critdam;

  damage = std::min(100, damage);
  ship.damage = std::min(100, (int)(ship.damage) + damage);

  auto [casualties, casualties1, primgundamage, secgundamage] =
      do_collateral(ship, damage);
  /* set laser strength for ships to maximum safe limit */
  if (ship.fire_laser) {
    int safe = (int)((1.0 - .01 * ship.damage) * ship.tech / 4.0);
    if (ship.fire_laser > safe) ship.fire_laser = safe;
  }

  if (penetrate) {
    msg << std::format(
        "\t\t{} penetrations  eff armor={} defense={} prob={:.3f}\n", penetrate,
        arm, defense, r);
  }
  if (crithits) {
    msg << std::format("\t\t{} CRITICAL hits do {}% damage\n", crithits,
                       critdam);
    msg << critmsg;
  }
  if (damage) {
    msg << std::format("\tDamage: {}% damage for a total of {}%\n", damage,
                       ship.damage);
  }
  if (primgundamage || secgundamage) {
    msg << std::format("\t Other: {} primary/{} secondary guns destroyed\n",
                       primgundamage, secgundamage);
  }
  if (casualties || casualties1) {
    msg << std::format("\tKilled: {} civ + {} mil casualties\n", casualties,
                       casualties1);
  }

  if (ship.damage >= 100) kill_ship(who, &ship);
  ship.build_cost = (int)cost(ship);
  return {damage, msg.str()};
}

/**
 * @brief Determines the disposition of a ship.
 *
 * This function calculates the values of evade, speed, and body based on the
 * given ship.
 *
 * @param ship The ship for which the disposition is being determined.
 * @return A tuple containing the evade value, speed, and body size of the ship.
 */
static std::tuple<int, int, int> ship_disposition(const Ship &ship) {
  int evade = 0;
  int speed = 0;
  int body = size(ship);
  if (ship.active && !ship.docked && (ship.whatdest || ship.navigate.on)) {
    evade = ship.protect.evade;
    speed = ship.speed;
  }
  return {evade, speed, body};
}

// TODO(jeffbailey): return bool.
static int CEW_hit(double dist, int cew_range) {
  int hits = 0;
  int prob = cew_hit_odds(dist, cew_range);

  if (success(prob)) hits = 1;

  return hits;
}

static int Num_hits(double dist, bool focus, int guns, double tech, int fdam,
                    int fev, int tev, int fspeed, int tspeed, int body,
                    guntype_t caliber, int defense, int *hit_probability) {
  auto [prob, factor] = hit_odds(dist, tech, fdam, fev, tev, fspeed, tspeed,
                                 body, caliber, defense);

  int hits = 0;
  if (focus) {
    if (success(prob * prob / 100)) hits = guns;
    *hit_probability = prob * prob / 100;
  } else {
    for (auto i = 1; i <= guns; i++)
      if (success(prob)) hits++;
    *hit_probability = prob;
  }

  return hits;
}

/**
 * @brief Calculates the odds of hitting a target and a range factor based on
 * combat parameters.
 *
 * This function computes the probability (odds) of a successful hit and a range
 * factor for a shot, given various parameters such as weapon technology,
 * damage, speeds, body size, gun caliber, and target defense.
 *
 * @param range     The distance to the target.
 * @param tech      The technology level of the shooter.
 * @param fdam      The damage factor of the firing entity (percentage, 0-100).
 * @param fev       The experience value of the firing entity.
 * @param tev       The experience value of the target entity.
 * @param fspeed    The speed of the firing entity.
 * @param tspeed    The speed of the target entity.
 * @param body      The body size of the target.
 * @param caliber   The caliber of the gun (guntype_t).
 * @param defense   The defense factor of the target (percentage, 0-100).
 * @return std::pair<int, int> A pair where the first element is the hit odds
 * (percentage), and the second element is the computed range factor.
 */
std::pair<int, int> hit_odds(double range, double tech, int fdam, int fev,
                             int tev, int fspeed, int tspeed, int body,
                             guntype_t caliber, int defense) {
  if (caliber == GTYPE_NONE) {
    return {0, 0};
  }

  double a =
      std::log10(1.0 + (double)tech) * 80.0 * std::pow((double)body, 0.33333);
  double b = 72.0 / ((2.0 + (double)tev) * (2.0 + (double)fev) *
                     (18.0 + (double)tspeed + (double)fspeed));
  double c = a * b / (double)caliber;
  int factor = (int)(c * (1.0 - (double)fdam / 100.)); /* 50% hit range */
  int odds = 0;
  if (factor > 0)
    odds = (int)((double)((factor) * 100) / ((double)((factor) + (int)range)));
  odds = (int)((double)odds * (1.0 - 0.1 * (double)defense));
  return {odds, factor};
}

static int cew_hit_odds(double range, int cew_range) {
  double factor =
      (range + 1.0) / ((double)cew_range + 1.0); /* maximum chance */
  int odds = (int)(100.0 *
                   std::exp((double)(-50.0 * (factor - 1.0) * (factor - 1.0))));
  return odds;
}

/*
 * range of telescopes, ground or space, given race and ship
 */
double tele_range(ShipType type, double tech) {
  if (type == ShipType::OTYPE_GTELE)
    return std::log1p((double)tech) * 400 + SYSTEMSIZE / 8;

  return std::log1p((double)tech) * 1500 + SYSTEMSIZE / 3;
}

guntype_t current_caliber(const Ship &ship) {
  if (ship.laser && ship.fire_laser) return GTYPE_LIGHT;
  if (ship.type == ShipType::STYPE_MINE) return GTYPE_LIGHT;
  if (ship.type == ShipType::STYPE_MISSILE) return GTYPE_HEAVY;
  if (ship.guns == PRIMARY) return ship.primtype;
  if (ship.guns == SECONDARY) return ship.sectype;

  return GTYPE_NONE;
}

static std::string do_critical_hits(int penetrate, Ship &ship, int *crithits,
                                    int *critdam, int caliber) {
  std::stringstream critmsg;
  *critdam = 0;
  *crithits = 0;
  int eff_size = std::max(1, shipbody(ship) / caliber);
  for (auto i = 1; i <= penetrate; i++)
    if (!int_rand(0, eff_size - 1)) {
      *crithits += 1;
      int dam = int_rand(0, 100);
      *critdam += dam;
    }
  *critdam = std::min(100, *critdam);
  /* check for special systems damage */
  critmsg << "\t\tSpecial systems damage: ";
  if (ship.cew && success(*critdam)) {
    critmsg << "CEW ";
    ship.cew = 0;
  }
  if (ship.laser && success(*critdam)) {
    critmsg << "Laser ";
    ship.laser = 0;
  }
  if (ship.cloak && success(*critdam)) {
    critmsg << "Cloak ";
    ship.cloak = 0;
  }
  if (ship.hyper_drive.has && success(*critdam)) {
    critmsg << "Hyper-drive ";
    ship.hyper_drive.has = 0;
  }
  if (ship.max_speed && success(*critdam)) {
    ship.speed = 0;
    ship.max_speed = int_rand(0, (int)ship.max_speed - 1);
    critmsg << std::format("Speed={} ", ship.max_speed);
  }
  if (ship.armor && success(*critdam)) {
    ship.armor = int_rand(0, (int)ship.armor - 1);
    critmsg << std::format("Armor={} ", ship.armor);
  }
  critmsg << "\n";
  return critmsg.str();
}

std::tuple<int, int, int, int> do_collateral(Ship &ship, int damage) {
  /* compute crew/troop casualties */
  int casualties = 0;
  int casualties1 = 0;
  int primgundamage = 0;
  int secgundamage = 0;

  for (auto i = 1; i <= ship.popn; i++) casualties += success(damage);
  ship.popn -= casualties;
  for (auto i = 1; i <= ship.troops; i++) casualties1 += success(damage);
  ship.troops -= casualties1;
  for (auto i = 1; i <= ship.primary; i++) primgundamage += success(damage);
  ship.primary -= primgundamage;
  for (auto i = 1; i <= ship.secondary; i++) secgundamage += success(damage);
  ship.secondary -= secgundamage;
  if (!ship.primary) ship.primtype = GTYPE_NONE;
  if (!ship.secondary) ship.sectype = GTYPE_NONE;
  return {casualties, casualties1, primgundamage, secgundamage};
}

static double p_factor(double attacker, double defender) {
  return ((2. / 3.141592) *
          std::atan(5 * (double)((attacker + 1.0) / (defender + 1.0))));
}

int planet_guns(long points) {
  if (points < 0) return 0; /* shouldn't happen */
  return std::min(20L, points / 1000);
}

static void mutate_sector(Sector &s) {
  if (int_rand(0, 6) >= Defensedata[s.condition]) s.condition = s.type;
}
