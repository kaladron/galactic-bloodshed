// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

#include "gb/buffers.h"

module gblib;

static int hit_probability;
static double penetration_factor;

static int do_radiation(Ship &ship, double tech, int strength, int hits,
                        const char *weapon, char *msg);
static int do_damage(player_t who, Ship &ship, double tech, int strength,
                     int hits, int defense, int caliber, double range,
                     const std::string_view weapon, char *msg);

static void ship_disposition(const Ship &ship, int *fevade, int *fspeed,
                             int *fbody);
static int CEW_hit(double dist, int cew_range);
static int Num_hits(double dist, bool focus, int strength, double tech,
                    int damage, int fevade, int tevade, int fspeed, int tspeed,
                    int tbody, int caliber, int defense);
static int cew_hit_odds(double dist, int cew_range);
static void do_critical_hits(int penetrate, Ship &ship, int *hits, int *damage,
                             int defense, char *msg);
static double p_factor(double tech, double penetration_factor);
static void mutate_sector(Sector &s);

int shoot_ship_to_ship(const Ship &from, Ship &to, int strength, int cew,
                       bool ignore, char *long_msg, char *short_msg) {
  int fevade;
  int fspeed;
  int fbody;
  int tevade;
  int tspeed;
  int tbody;
  char weapon[32];
  char damage_msg[1024];

  if (strength <= 0) return -1;

  if (!(from.alive || ignore) || !to.alive) return -1;
  if (from.whatorbits == ScopeLevel::LEVEL_SHIP ||
      from.whatorbits == ScopeLevel::LEVEL_UNIV)
    return -1;
  if (to.whatorbits == ScopeLevel::LEVEL_SHIP ||
      to.whatorbits == ScopeLevel::LEVEL_UNIV)
    return -1;
  if (from.storbits != to.storbits) return -1;
  if (has_switch(from) && !from.on) return -1;

  double xfrom = from.xpos;
  double yfrom = from.ypos;

  double xto = to.xpos;
  double yto = to.ypos;
  /* compute caliber */
  int caliber = current_caliber(from);

  double dist = [xfrom, yfrom, xto, yto, &from]() -> double {
    if (from.type ==
        ShipType::STYPE_MISSILE) /* missiles hit at point blank range */
      return 0.0;
    else {
      double dist = sqrt((double)Distsq(xfrom, yfrom, xto, yto));
      if (from.type == ShipType::STYPE_MINE) { /* compute the effective range */
        dist *= dist / 200.0; /* mines are very effective inside 200 */
      }
      return dist;
    }
  }();

  if ((double)dist > gun_range(from)) return -1;

  /* attack parameters */
  ship_disposition(from, &fevade, &fspeed, &fbody);
  ship_disposition(to, &tevade, &tspeed, &tbody);
  auto defense = getdefense(to);

  bool focus = (laser_on(from) && from.focus) ? true : false;

  int hits = cew != 0 ? strength * CEW_hit((double)dist, (int)from.cew_range)
                      : Num_hits((double)dist, focus, strength, from.tech,
                                 (int)from.damage, fevade, tevade, fspeed,
                                 tspeed, tbody, caliber, defense);

  /* CEW, destruct, lasers */
  int damage = 0;
  if (from.mode) {
    damage =
        do_radiation(to, from.tech, strength, hits, "radiation", damage_msg);
    sprintf(short_msg, "%s: %s %s %s\n", dispshiploc(to).c_str(),
            ship_to_string(from).c_str(), to.alive ? "attacked" : "DESTROYED",
            ship_to_string(to).c_str());
    strcpy(long_msg, short_msg);
    strcat(long_msg, damage_msg);
  } else {
    if (cew)
      sprintf(weapon, "strength CEW");
    else if (laser_on(from)) {
      if (from.focus)
        sprintf(weapon, "strength focused laser");
      else
        sprintf(weapon, "strength laser");
    } else
      switch (caliber) {
        case GTYPE_LIGHT:
          sprintf(weapon, "light guns");
          break;
        case GTYPE_MEDIUM:
          sprintf(weapon, "medium guns");
          break;
        case GTYPE_HEAVY:
          sprintf(weapon, "heavy guns");
          break;
        case GTYPE_NONE:
          sprintf(weapon, "pea-shooter");
          return -1;
      }

    damage = do_damage(from.owner, to, (double)from.tech, strength, hits,
                       defense, caliber, (double)dist, weapon, damage_msg);
    sprintf(short_msg, "%s: %s %s %s\n", dispshiploc(to).c_str(),
            ship_to_string(from).c_str(), to.alive ? "attacked" : "DESTROYED",
            ship_to_string(to).c_str());
    strcpy(long_msg, short_msg);
    strcat(long_msg, damage_msg);
  }
  return damage;
}

int shoot_planet_to_ship(Race &race, Ship &ship, int strength, char *long_msg,
                         char *short_msg) {
  int evade;
  int speed;
  int body;
  char damage_msg[1024];

  int hits = 0;
  if (strength <= 0) return -1;
  if (!ship.alive) return -1;

  if (ship.whatorbits != ScopeLevel::LEVEL_PLAN) return -1;

  ship_disposition(ship, &evade, &speed, &body);

  hits = Num_hits(0.0, false, strength, race.tech, 0, evade, 0, speed, 0, body,
                  GTYPE_MEDIUM, 1);

  int damage = do_damage(race.Playernum, ship, race.tech, strength, hits, 0,
                         GTYPE_MEDIUM, 0.0, "medium guns", damage_msg);
  sprintf(short_msg, "%s [%d] %s %s\n", dispshiploc(ship).c_str(),
          race.Playernum, ship.alive ? "attacked" : "DESTROYED",
          ship_to_string(ship).c_str());
  strcpy(long_msg, short_msg);
  strcat(long_msg, damage_msg);

  return damage;
}

/**
 * @return Number of sectors destroyed.
 */
int shoot_ship_to_planet(Ship *ship, Planet &pl, int strength, int x, int y,
                         SectorMap &smap, int ignore, int caliber,
                         char *long_msg, char *short_msg) {
  int numdest = 0;

  if (strength <= 0) return -1;
  if (!(ship->alive || ignore)) return -1;
  if (has_switch(*ship) && !ship->on) return -1;
  if (ship->whatorbits != ScopeLevel::LEVEL_PLAN) return -1;

  if (x < 0 || x > pl.Maxx - 1 || y < 0 || y > pl.Maxy - 1) return -1;

  double r = .4 * strength;
  if (!caliber) { /* figure out the appropriate gun caliber if not given*/
    if (ship->fire_laser)
      caliber = GTYPE_LIGHT;
    else
      switch (ship->guns) {
        case PRIMARY:
          caliber = ship->primtype;
          break;
        case SECONDARY:
          caliber = ship->sectype;
          break;
        default:
          caliber = GTYPE_LIGHT;
      }
  }

  auto &target = smap.get(x, y);
  player_t oldowner = target.owner;

  std::array<int, MAXPLAYERS> sum_mob{0};

  for (auto y2 = 0; y2 < pl.Maxy; y2++) {
    for (auto x2 = 0; x2 < pl.Maxx; x2++) {
      int dx = std::min(abs(x2 - x), abs(x + (pl.Maxx - 1) - x2));
      int dy = abs(y2 - y);
      double d = sqrt((double)(dx * dx + dy * dy));
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
  auto num_sectors = pl.Maxx * pl.Maxy;
  for (auto i = 1; i <= Num_races; i++) {
    pl.info[i - 1].mob_points = sum_mob[i - 1];
    pl.info[i - 1].comread = sum_mob[i - 1] / num_sectors;
    pl.info[i - 1].guns = planet_guns(sum_mob[i - 1]);
  }

  /* planet toxicity goes up a bit */
  pl.conditions[TOXIC] += (100 - pl.conditions[TOXIC]) *
                          ((double)numdest / (double)(pl.Maxx * pl.Maxy));

  sprintf(short_msg, "%s bombards %s [%d]\n", ship_to_string(*ship).c_str(),
          dispshiploc(*ship).c_str(), oldowner);
  strcpy(long_msg, short_msg);
  sprintf(buf, "\t%d sectors destroyed\n", numdest);
  strcat(long_msg, buf);
  return numdest;
}

static int do_radiation(Ship &ship, double tech, int strength, int hits,
                        const char *weapon, char *msg) {
  double fac =
      (2. / 3.14159265) * atan((double)(5 * (tech + 1.0) / (ship.tech + 1.0)));

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
  sprintf(buf, "\tAttack: %d %s\n\t  Hits: %d\n", strength, weapon, hits);
  strcat(msg, buf);
  sprintf(buf, "\t   Rad: %d%% for a total of %d%%\n", dosage, ship.rad);
  strcat(msg, buf);
  if (casualties || casualties1) {
    sprintf(buf, "\tKilled: %d civ + %d mil\n", casualties, casualties1);
    strcat(msg, buf);
  }
  return dosage;
}

static int do_damage(player_t who, Ship &ship, double tech, int strength,
                     int hits, int defense, int caliber, double range,
                     const std::string_view weapon, char *msg) {
  char critmsg[1024];

  sprintf(buf, "\tAttack: %d %s at a range of %.0f\n", strength, weapon.data(),
          range);
  strcpy(msg, buf);
  sprintf(buf, "\t  Hits: %d  %d%% probability\n", hits, hit_probability);
  strcat(msg, buf);
  /* ship may lose some armor */
  if (ship.armor)
    if (success(hits * caliber)) {
      ship.armor--;
      sprintf(buf, "\t\tArmor reduced to %d\n", ship.armor);
      strcat(msg, buf);
    }

  double fac = p_factor(tech, ship.tech);
  penetration_factor = fac;
  int arm = std::max(0UL, armor(ship) + defense - hits / 5);
  double body = sqrt((double)(0.1 * shipbody(ship)));

  int critdam = 0;
  int crithits = 0;
  int penetrate = 0;
  double r = 1.0;
  for (int i = 1; i <= arm; i++) r *= fac;

  for (int i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r) penetrate += 1;

  int damage = round_rand(SHIP_DAMAGE * (double)caliber * (double)penetrate /
                          (double)body);

  do_critical_hits(penetrate, ship, &crithits, &critdam, caliber, critmsg);

  if (crithits) damage += critdam;

  damage = std::min(100, damage);
  ship.damage = std::min(100, (int)(ship.damage) + damage);

  int casualties;
  int casualties1;
  int primgundamage;
  int secgundamage;
  do_collateral(&ship, damage, &casualties, &casualties1, &primgundamage,
                &secgundamage);
  /* set laser strength for ships to maximum safe limit */
  if (ship.fire_laser) {
    int safe = (int)((1.0 - .01 * ship.damage) * ship.tech / 4.0);
    if (ship.fire_laser > safe) ship.fire_laser = safe;
  }

  if (penetrate) {
    sprintf(buf, "\t\t%d penetrations  eff armor=%d defense=%d prob=%.3f\n",
            penetrate, arm, defense, r);
    strcat(msg, buf);
  }
  if (crithits) {
    sprintf(buf, "\t\t%d CRITICAL hits do %d%% damage\n", crithits, critdam);
    strcat(msg, buf);
    strcat(msg, critmsg);
  }
  if (damage) {
    sprintf(buf, "\tDamage: %d%% damage for a total of %d%%\n", damage,
            ship.damage);
    strcat(msg, buf);
  }
  if (primgundamage || secgundamage) {
    sprintf(buf, "\t Other: %d primary/%d secondary guns destroyed\n",
            primgundamage, secgundamage);
    strcat(msg, buf);
  }
  if (casualties || casualties1) {
    sprintf(buf, "\tKilled: %d civ + %d mil casualties\n", casualties,
            casualties1);
    strcat(msg, buf);
  }

  if (ship.damage >= 100) kill_ship(who, &ship);
  ship.build_cost = (int)cost(ship);
  return damage;
}

static void ship_disposition(const Ship &ship, int *evade, int *speed,
                             int *body) {
  *evade = 0;
  *speed = 0;
  *body = size(ship);
  if (ship.active && !ship.docked && (ship.whatdest || ship.navigate.on)) {
    *evade = ship.protect.evade;
    *speed = ship.speed;
  }
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
                    int caliber, int defense) {
  int factor;

  int prob = hit_odds(dist, &factor, tech, fdam, fev, tev, fspeed, tspeed, body,
                      caliber, defense);

  int hits = 0;
  if (focus) {
    if (success(prob * prob / 100)) hits = guns;
    hit_probability = prob * prob / 100;
  } else {
    for (auto i = 1; i <= guns; i++)
      if (success(prob)) hits++;
    hit_probability = prob; /* global variable */
  }

  return hits;
}

int hit_odds(double range, int *factor, double tech, int fdam, int fev, int tev,
             int fspeed, int tspeed, int body, int caliber, int defense) {
  if (caliber == GTYPE_NONE) {
    *factor = 0;
    return 0;
  }

  double a = log10(1.0 + (double)tech) * 80.0 * pow((double)body, 0.33333);
  double b = 72.0 / ((2.0 + (double)tev) * (2.0 + (double)fev) *
                     (18.0 + (double)tspeed + (double)fspeed));
  double c = a * b / (double)caliber;
  *factor = (int)(c * (1.0 - (double)fdam / 100.)); /* 50% hit range */
  int odds = 0;
  if (*factor > 0)
    odds =
        (int)((double)((*factor) * 100) / ((double)((*factor) + (int)range)));
  odds = (int)((double)odds * (1.0 - 0.1 * (double)defense));
  return odds;
}

static int cew_hit_odds(double range, int cew_range) {
  double factor =
      (range + 1.0) / ((double)cew_range + 1.0); /* maximum chance */
  int odds =
      (int)(100.0 * exp((double)(-50.0 * (factor - 1.0) * (factor - 1.0))));
  return odds;
}

/*
 * range of telescopes, ground or space, given race and ship
 */
double tele_range(int type, double tech) {
  if (type == ShipType::OTYPE_GTELE)
    return log1p((double)tech) * 400 + SYSTEMSIZE / 8;

  return log1p((double)tech) * 1500 + SYSTEMSIZE / 3;
}

int current_caliber(const Ship &ship) {
  if (ship.laser && ship.fire_laser) return GTYPE_LIGHT;
  if (ship.type == ShipType::STYPE_MINE) return GTYPE_LIGHT;
  if (ship.type == ShipType::STYPE_MISSILE) return GTYPE_HEAVY;
  if (ship.guns == PRIMARY) return ship.primtype;
  if (ship.guns == SECONDARY) return ship.sectype;

  return GTYPE_NONE;
}

static void do_critical_hits(int penetrate, Ship &ship, int *crithits,
                             int *critdam, int caliber, char *critmsg) {
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
  strcpy(critmsg, "\t\tSpecial systems damage: ");
  if (ship.cew && success(*critdam)) {
    strcat(critmsg, "CEW ");
    ship.cew = 0;
  }
  if (ship.laser && success(*critdam)) {
    strcat(critmsg, "Laser ");
    ship.laser = 0;
  }
  if (ship.cloak && success(*critdam)) {
    strcat(critmsg, "Cloak ");
    ship.cloak = 0;
  }
  if (ship.hyper_drive.has && success(*critdam)) {
    strcat(critmsg, "Hyper-drive ");
    ship.hyper_drive.has = 0;
  }
  if (ship.max_speed && success(*critdam)) {
    ship.speed = 0;
    ship.max_speed = int_rand(0, (int)ship.max_speed - 1);
    sprintf(buf, "Speed=%d ", ship.max_speed);
    strcat(critmsg, buf);
  }
  if (ship.armor && success(*critdam)) {
    ship.armor = int_rand(0, (int)ship.armor - 1);
    sprintf(buf, "Armor=%d ", ship.armor);
    strcat(critmsg, buf);
  }
  strcat(critmsg, "\n");
}

void do_collateral(Ship *ship, int damage, int *casualties, int *casualties1,
                   int *primgundamage, int *secgundamage) {
  /* compute crew/troop casualties */
  *casualties = 0;
  *casualties1 = 0;
  *primgundamage = 0;
  *secgundamage = 0;

  for (auto i = 1; i <= ship->popn; i++) *casualties += success(damage);
  ship->popn -= *casualties;
  for (auto i = 1; i <= ship->troops; i++) *casualties1 += success(damage);
  ship->troops -= *casualties1;
  for (auto i = 1; i <= ship->primary; i++) *primgundamage += success(damage);
  ship->primary -= *primgundamage;
  for (auto i = 1; i <= ship->secondary; i++) *secgundamage += success(damage);
  ship->secondary -= *secgundamage;
  if (!ship->primary) ship->primtype = GTYPE_NONE;
  if (!ship->secondary) ship->sectype = GTYPE_NONE;
}

static double p_factor(double attacker, double defender) {
  return ((2. / 3.141592) *
          atan(5 * (double)((attacker + 1.0) / (defender + 1.0))));
}

int planet_guns(long points) {
  if (points < 0) return 0; /* shouldn't happen */
  return std::min(20L, points / 1000);
}

static void mutate_sector(Sector &s) {
  if (int_rand(0, 6) >= Defensedata[s.condition]) s.condition = s.type;
}
