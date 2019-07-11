// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/shootblast.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/build.h"
#include "gb/defense.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/getplace.h"
#include "gb/misc.h"
#include "gb/races.h"
#include "gb/rand.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static int hit_probability;
static double penetration_factor;

static int do_radiation(Ship *, double, int, int, const char *, char *);
static int do_damage(int, Ship *, double, int, int, int, int, double,
                     const char *, char *);

static void ship_disposition(Ship *, int *, int *, int *);
static int CEW_hit(double, int);
static int Num_hits(double, int, int, double, int, int, int, int, int, int, int,
                    int);
static int cew_hit_odds(double, int);
static void do_critical_hits(int, Ship *, int *, int *, int, char *);
static double p_factor(double, double);
static void mutate_sector(Sector &);

int shoot_ship_to_ship(Ship *from, Ship *to, int strength, int cew, int ignore,
                       char *long_msg, char *short_msg) {
  int hits;
  int damage;
  int defense;
  double dist;
  double xfrom;
  double yfrom;
  double xto;
  double yto;
  int focus;
  int fevade;
  int fspeed;
  int fbody;
  int tevade;
  int tspeed;
  int tbody;
  int caliber;
  char weapon[32];
  char damage_msg[1024];

  hits = 0;
  defense = 1;
  if (strength <= 0) return -1;

  if (!(from->alive || ignore) || !to->alive) return -1;
  if (from->whatorbits == ScopeLevel::LEVEL_SHIP ||
      from->whatorbits == ScopeLevel::LEVEL_UNIV)
    return -1;
  if (to->whatorbits == ScopeLevel::LEVEL_SHIP ||
      to->whatorbits == ScopeLevel::LEVEL_UNIV)
    return -1;
  if (from->storbits != to->storbits) return -1;
  if (has_switch(from) && !from->on) return -1;

  xfrom = from->xpos;
  yfrom = from->ypos;

  xto = to->xpos;
  yto = to->ypos;
  /* compute caliber */
  caliber = current_caliber(from);

  if (from->type ==
      ShipType::STYPE_MISSILE) /* missiles hit at point blank range */
    dist = 0.0;
  else {
    dist = sqrt((double)Distsq(xfrom, yfrom, xto, yto));
    if (from->type == ShipType::STYPE_MINE) { /* compute the effective range */
      dist *= dist / 200.0; /* mines are very effective inside 200 */
    }
  }
  if ((double)dist > gun_range((Race *)nullptr, from, 0)) return -1;
  /* attack parameters */
  ship_disposition(from, &fevade, &fspeed, &fbody);
  ship_disposition(to, &tevade, &tspeed, &tbody);
  defense = getdefense(to);

  if (laser_on(from) && from->focus)
    focus = 1;
  else
    focus = 0;

  if (cew)
    hits = strength * CEW_hit((double)dist, (int)from->cew_range);
  else
    hits =
        Num_hits((double)dist, focus, strength, from->tech, (int)from->damage,
                 fevade, tevade, fspeed, tspeed, tbody, caliber, defense);
  /* CEW, destruct, lasers */
  damage = 0;
  if (from->mode) {
    damage =
        do_radiation(to, from->tech, strength, hits, "radiation", damage_msg);
    sprintf(short_msg, "%s: %s %s %s\n", Dispshiploc(to),
            ship_to_string(*from).c_str(), to->alive ? "attacked" : "DESTROYED",
            ship_to_string(*to).c_str());
    strcpy(long_msg, short_msg);
    strcat(long_msg, damage_msg);
  } else {
    if (cew)
      sprintf(weapon, "strength CEW");
    else if (laser_on(from)) {
      if (from->focus)
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

    damage = do_damage((int)from->owner, to, (double)from->tech, strength, hits,
                       defense, caliber, (double)dist, weapon, damage_msg);
    sprintf(short_msg, "%s: %s %s %s\n", Dispshiploc(to),
            ship_to_string(*from).c_str(), to->alive ? "attacked" : "DESTROYED",
            ship_to_string(*to).c_str());
    strcpy(long_msg, short_msg);
    strcat(long_msg, damage_msg);
  }
  return damage;
}

#ifdef DEFENSE
int shoot_planet_to_ship(racetype *Race, Ship *ship, int strength,
                         char *long_msg, char *short_msg) {
  int hits;
  int evade;
  int speed;
  int body;
  int damage;
  char damage_msg[1024];

  hits = 0;
  if (strength <= 0) return -1;
  if (!ship->alive) return -1;

  if (ship->whatorbits != ScopeLevel::LEVEL_PLAN) return -1;

  ship_disposition(ship, &evade, &speed, &body);

  hits = Num_hits(0.0, 0, strength, Race->tech, 0, evade, 0, speed, 0, body,
                  GTYPE_MEDIUM, 1);

  damage = do_damage(Race->Playernum, ship, Race->tech, strength, hits, 0,
                     GTYPE_MEDIUM, 0.0, "medium guns", damage_msg);
  sprintf(short_msg, "%s [%d] %s %s\n", Dispshiploc(ship), Race->Playernum,
          ship->alive ? "attacked" : "DESTROYED",
          ship_to_string(*ship).c_str());
  strcpy(long_msg, short_msg);
  strcat(long_msg, damage_msg);

  return damage;
}
#endif

int shoot_ship_to_planet(Ship *ship, Planet *pl, int strength, int x, int y,
                         SectorMap &smap, int ignore, int caliber,
                         char *long_msg, char *short_msg) {
  int x2;
  int y2;
  int numdest;
  player_t oldowner;
  population_t kills;
  int i;
  int num_sectors;
  int sum_mob[MAXPLAYERS];
  double d;
  double r;
  double fac;

  numdest = 0;
  if (strength <= 0) return -1;
  if (!(ship->alive || ignore)) return -1;
  if (has_switch(ship) && !ship->on) return -1;
  if (ship->whatorbits != ScopeLevel::LEVEL_PLAN) return -1;

  if (x < 0 || x > pl->Maxx - 1 || y < 0 || y > pl->Maxy - 1) return -1;

  r = .4 * strength;
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
  oldowner = target.owner;

  for (i = 1; i <= Num_races; i++) sum_mob[i - 1] = 0;

  for (y2 = 0; y2 < pl->Maxy; y2++) {
    for (x2 = 0; x2 < pl->Maxx; x2++) {
      int dx;
      int dy;
      dx = std::min(abs(x2 - x), abs(x + (pl->Maxx - 1) - x2));
      dy = abs(y2 - y);
      d = sqrt((double)(dx * dx + dy * dy));
      auto &s = smap.get(x2, y2);

      if (d <= r) {
        fac = SECTOR_DAMAGE * (double)strength * (double)caliber / (d + 1.);

        if (s.owner) {
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
  num_sectors = pl->Maxx * pl->Maxy;
  for (i = 1; i <= Num_races; i++) {
    pl->info[i - 1].mob_points = sum_mob[i - 1];
    pl->info[i - 1].comread = sum_mob[i - 1] / num_sectors;
    pl->info[i - 1].guns = planet_guns(sum_mob[i - 1]);
  }

  /* planet toxicity goes up a bit */
  pl->conditions[TOXIC] += (100 - pl->conditions[TOXIC]) *
                           ((double)numdest / (double)(pl->Maxx * pl->Maxy));

  sprintf(short_msg, "%s bombards %s [%d]\n", ship_to_string(*ship).c_str(),
          Dispshiploc(ship), oldowner);
  strcpy(long_msg, short_msg);
  sprintf(buf, "\t%d sectors destroyed\n", numdest);
  strcat(long_msg, buf);
  return numdest;
}

static int do_radiation(Ship *ship, double tech, int strength, int hits,
                        const char *weapon, char *msg) {
  double fac;
  double r;
  int i;
  int arm;
  int body;
  int penetrate;
  int casualties;
  int casualties1;
  int dosage;

  fac =
      (2. / 3.14159265) * atan((double)(5 * (tech + 1.0) / (ship->tech + 1.0)));

  arm = std::max(0UL, Armor(ship) - hits / 5);
  body = Body(ship);

  penetrate = 0;
  r = 1.0;
  for (i = 1; i <= arm; i++) r *= fac;

  for (i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r) penetrate += 1;

  dosage = round_rand(40. * (double)penetrate / (double)body);
  dosage = std::min(100, dosage);

  if (dosage > ship->rad) ship->rad = std::max(ship->rad, dosage);
  if (success(ship->rad)) ship->active = 0;

  casualties = 0;
  casualties1 = 0;
  sprintf(buf, "\tAttack: %d %s\n\t  Hits: %d\n", strength, weapon, hits);
  strcat(msg, buf);
  sprintf(buf, "\t   Rad: %d%% for a total of %d%%\n", dosage, ship->rad);
  strcat(msg, buf);
  if (casualties || casualties1) {
    sprintf(buf, "\tKilled: %d civ + %d mil\n", casualties, casualties1);
    strcat(msg, buf);
  }
  return dosage;
}

static int do_damage(int who, Ship *ship, double tech, int strength, int hits,
                     int defense, int caliber, double range, const char *weapon,
                     char *msg) {
  double body;
  int i;
  int arm;
  int damage;
  int penetrate;
  int crithits;
  int critdam;
  int casualties;
  int casualties1;
  int primgundamage;
  int secgundamage;
  double fac;
  double r;
  char critmsg[1024];

  sprintf(buf, "\tAttack: %d %s at a range of %.0f\n", strength, weapon, range);
  strcpy(msg, buf);
  sprintf(buf, "\t  Hits: %d  %d%% probability\n", hits, hit_probability);
  strcat(msg, buf);
  /* ship may lose some armor */
  if (ship->armor)
    if (success(hits * caliber)) {
      ship->armor--;
      sprintf(buf, "\t\tArmor reduced to %d\n", ship->armor);
      strcat(msg, buf);
    }

  fac = p_factor(tech, ship->tech);
  penetration_factor = fac;
  arm = std::max(0UL, Armor(ship) + defense - hits / 5);
  body = sqrt((double)(0.1 * Body(ship)));

  critdam = 0;
  crithits = 0;
  penetrate = 0;
  r = 1.0;
  for (i = 1; i <= arm; i++) r *= fac;

  for (i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r) penetrate += 1;

  damage = round_rand(SHIP_DAMAGE * (double)caliber * (double)penetrate /
                      (double)body);

  do_critical_hits(penetrate, ship, &crithits, &critdam, caliber, critmsg);

  if (crithits) damage += critdam;

  damage = std::min(100, damage);
  ship->damage = std::min(100, (int)(ship->damage) + damage);
  do_collateral(ship, damage, &casualties, &casualties1, &primgundamage,
                &secgundamage);
  /* set laser strength for ships to maximum safe limit */
  if (ship->fire_laser) {
    int safe;
    safe = (int)((1.0 - .01 * ship->damage) * ship->tech / 4.0);
    if (ship->fire_laser > safe) ship->fire_laser = safe;
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
            ship->damage);
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

  if (ship->damage >= 100) kill_ship(who, ship);
  ship->build_cost = (int)cost(ship);
  return damage;
}

static void ship_disposition(Ship *ship, int *evade, int *speed, int *body) {
  *evade = 0;
  *speed = 0;
  *body = Size(ship);
  if (ship->active && !ship->docked && (ship->whatdest || ship->navigate.on)) {
    *evade = ship->protect.evade;
    *speed = ship->speed;
  }
}

static int CEW_hit(double dist, int cew_range) {
  int prob;
  int hits;

  hits = 0;
  prob = cew_hit_odds(dist, cew_range);

  if (success(prob)) hits = 1;

  return hits;
}

static int Num_hits(double dist, int focus, int guns, double tech, int fdam,
                    int fev, int tev, int fspeed, int tspeed, int body,
                    int caliber, int defense) {
  int factor;
  int i;
  int prob;
  int hits;

  prob = hit_odds(dist, &factor, tech, fdam, fev, tev, fspeed, tspeed, body,
                  caliber, defense);

  hits = 0;
  if (focus) {
    if (success(prob * prob / 100)) hits = guns;
    hit_probability = prob * prob / 100;
  } else {
    for (i = 1; i <= guns; i++)
      if (success(prob)) hits++;
    hit_probability = prob; /* global variable */
  }

  return hits;
}

int hit_odds(double range, int *factor, double tech, int fdam, int fev, int tev,
             int fspeed, int tspeed, int body, int caliber, int defense) {
  int odds;
  double a;
  double b;
  double c;

  if (caliber == GTYPE_NONE) {
    *factor = 0;
    return 0;
  }

  a = log10(1.0 + (double)tech) * 80.0 * pow((double)body, 0.33333);
  b = 72.0 / ((2.0 + (double)tev) * (2.0 + (double)fev) *
              (18.0 + (double)tspeed + (double)fspeed));
  c = a * b / (double)caliber;
  *factor = (int)(c * (1.0 - (double)fdam / 100.)); /* 50% hit range */
  odds = 0;
  if (*factor > 0)
    odds =
        (int)((double)((*factor) * 100) / ((double)((*factor) + (int)range)));
  odds = (int)((double)odds * (1.0 - 0.1 * (double)defense));
  return odds;
}

static int cew_hit_odds(double range, int cew_range) {
  int odds;
  double factor;

  factor = (range + 1.0) / ((double)cew_range + 1.0); /* maximum chance */
  odds = (int)(100.0 * exp((double)(-50.0 * (factor - 1.0) * (factor - 1.0))));
  return odds;
}

/*
 * gun range of given ship, given race and ship
 */
double gun_range(Race *r, Ship *s, int mode) {
  if (mode) return (logscale((int)(r->tech + 1.0)) * SYSTEMSIZE);

  return (logscale((int)(s->tech + 1.0)) * SYSTEMSIZE);
}

/*
 * range of telescopes, ground or space, given race and ship
 */
double tele_range(int type, double tech) {
  if (type == ShipType::OTYPE_GTELE)
    return log1p((double)tech) * 400 + SYSTEMSIZE / 8;

  return log1p((double)tech) * 1500 + SYSTEMSIZE / 3;
}

int current_caliber(Ship *ship) {
  if (ship->laser && ship->fire_laser) return GTYPE_LIGHT;
  if (ship->type == ShipType::STYPE_MINE) return GTYPE_LIGHT;
  if (ship->type == ShipType::STYPE_MISSILE) return GTYPE_HEAVY;
  if (ship->guns == PRIMARY) return ship->primtype;
  if (ship->guns == SECONDARY) return ship->sectype;

  return GTYPE_NONE;
}

static void do_critical_hits(int penetrate, Ship *ship, int *crithits,
                             int *critdam, int caliber, char *critmsg) {
  int eff_size;
  int i;
  int dam;
  *critdam = 0;
  *crithits = 0;
  eff_size = std::max(1, Body(ship) / caliber);
  for (i = 1; i <= penetrate; i++)
    if (!int_rand(0, eff_size - 1)) {
      *crithits += 1;
      dam = int_rand(0, 100);
      *critdam += dam;
    }
  *critdam = std::min(100, *critdam);
  /* check for special systems damage */
  strcpy(critmsg, "\t\tSpecial systems damage: ");
  if (ship->cew && success(*critdam)) {
    strcat(critmsg, "CEW ");
    ship->cew = 0;
  }
  if (ship->laser && success(*critdam)) {
    strcat(critmsg, "Laser ");
    ship->laser = 0;
  }
  if (ship->cloak && success(*critdam)) {
    strcat(critmsg, "Cloak ");
    ship->cloak = 0;
  }
  if (ship->hyper_drive.has && success(*critdam)) {
    strcat(critmsg, "Hyper-drive ");
    ship->hyper_drive.has = 0;
  }
  if (ship->max_speed && success(*critdam)) {
    ship->speed = 0;
    ship->max_speed = int_rand(0, (int)ship->max_speed - 1);
    sprintf(buf, "Speed=%d ", ship->max_speed);
    strcat(critmsg, buf);
  }
  if (ship->armor && success(*critdam)) {
    ship->armor = int_rand(0, (int)ship->armor - 1);
    sprintf(buf, "Armor=%d ", ship->armor);
    strcat(critmsg, buf);
  }
  strcat(critmsg, "\n");
}

void do_collateral(Ship *ship, int damage, int *casualties, int *casualties1,
                   int *primgundamage, int *secgundamage) {
  int i;
  /* compute crew/troop casualties */
  *casualties = 0;
  *casualties1 = 0;
  *primgundamage = 0;
  *secgundamage = 0;

  for (i = 1; i <= ship->popn; i++) *casualties += success(damage);
  ship->popn -= *casualties;
  for (i = 1; i <= ship->troops; i++) *casualties1 += success(damage);
  ship->troops -= *casualties1;
  for (i = 1; i <= ship->primary; i++) *primgundamage += success(damage);
  ship->primary -= *primgundamage;
  for (i = 1; i <= ship->secondary; i++) *secgundamage += success(damage);
  ship->secondary -= *secgundamage;
  if (!ship->primary) ship->primtype = GTYPE_NONE;
  if (!ship->secondary) ship->sectype = GTYPE_NONE;
}

int getdefense(Ship *ship) {
  int defense = 0;

  if (landed(*ship)) {
    const auto p = getplanet(ship->storbits, ship->pnumorbits);
    auto sect = getsector(p, ship->land_x, ship->land_y);
    defense = 2 * Defensedata[sect.condition];
  }
  return defense;
}

static double p_factor(double attacker, double defender) {
  return ((2. / 3.141592) *
          atan(5 * (double)((attacker + 1.0) / (defender + 1.0))));
}

int planet_guns(int points) {
  if (points < 0) return 0; /* shouldn't happen */
  return std::min(20, points / 1000);
}

static void mutate_sector(Sector &s) {
  if (int_rand(0, 6) >= Defensedata[s.condition]) s.condition = s.type;
}
