#include <math.h>
#include <string.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

#include "GB_server.h"
#include "misc.h"

extern int Defensedata[];

int hit_probability;
double penetration_factor;

int shoot_ship_to_ship(shiptype *, shiptype *, int, int, int, char *, char *);
#ifdef DEFENSE
int shoot_planet_to_ship(racetype *, planettype *, shiptype *, int, char *,
                         char *);
#endif
int shoot_ship_to_planet(shiptype *, planettype *, int, int, int, int, int, int,
                         char *, char *);
int do_radiation(shiptype *, double, int, int, char *, char *);
int do_damage(int, shiptype *, double, int, int, int, int, double, char *,
              char *);
void ship_disposition(shiptype *, int *, int *, int *);
int CEW_hit(double, int);
int Num_hits(double, int, int, double, int, int, int, int, int, int, int, int);
int hit_odds(double, int *, double, int, int, int, int, int, int, int, int);
int cew_hit_odds(double, int);
double gun_range(racetype *, shiptype *, int);
double tele_range(int, double);
int current_caliber(shiptype *);
void do_critical_hits(int, shiptype *, int *, int *, int, char *);
void do_collateral(shiptype *, int, int *, int *, int *, int *);
int getdefense(shiptype *);
double p_factor(double, double);
int planet_guns(int);
void mutate_sector(sectortype *);
#include "fire.h"
#include "getplace.h"
#include "shlmisc.h"
#include "files_shl.h"
#include "rand.h"
#include "build.h"

int shoot_ship_to_ship(shiptype *from, shiptype *to, int strength, int cew,
                       int ignore, char *long_msg, char *short_msg) {
  int hits;
  int damage, defense;
  double dist, xfrom, yfrom, xto, yto;
  int focus, fevade, fspeed, fbody, tevade, tspeed, tbody, caliber;
  char weapon[32], damage_msg[1024];

  hits = 0;
  defense = 1;
  if (strength <= 0)
    return -1;

  if (!(from->alive || ignore) || !to->alive)
    return -1;
  if (from->whatorbits == LEVEL_SHIP || from->whatorbits == LEVEL_UNIV)
    return -1;
  if (to->whatorbits == LEVEL_SHIP || to->whatorbits == LEVEL_UNIV)
    return -1;
  if (from->storbits != to->storbits)
    return -1;
  if (has_switch(from) && !from->on)
    return -1;

  xfrom = from->xpos;
  yfrom = from->ypos;

  xto = to->xpos;
  yto = to->ypos;
  /* compute caliber */
  caliber = current_caliber(from);

  if (from->type == STYPE_MISSILE) /* missiles hit at point blank range */
    dist = 0.0;
  else {
    dist = sqrt((double)Distsq(xfrom, yfrom, xto, yto));
    if (from->type == STYPE_MINE) { /* compute the effective range */
      dist *= dist / 200.0;         /* mines are very effective inside 200 */
    }
  }
  if ((double)dist > gun_range((racetype *)NULL, from, 0))
    return -1;
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
    sprintf(short_msg, "%s: %s %s %s\n", Dispshiploc(to), Ship(from),
            to->alive ? "attacked" : "DESTROYED", Ship(to));
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
      case LIGHT:
        sprintf(weapon, "light guns");
        break;
      case MEDIUM:
        sprintf(weapon, "medium guns");
        break;
      case HEAVY:
        sprintf(weapon, "heavy guns");
        break;
      default:
        sprintf(weapon, "pea-shooter");
        return -1;
      }

    damage = do_damage((int)from->owner, to, (double)from->tech, strength, hits,
                       defense, caliber, (double)dist, weapon, damage_msg);
    sprintf(short_msg, "%s: %s %s %s\n", Dispshiploc(to), Ship(from),
            to->alive ? "attacked" : "DESTROYED", Ship(to));
    strcpy(long_msg, short_msg);
    strcat(long_msg, damage_msg);
  }
  return damage;
}

#ifdef DEFENSE
int shoot_planet_to_ship(racetype *Race, planettype *p, shiptype *ship,
                         int strength, char *long_msg, char *short_msg) {
  int hits;
  int evade, speed, body;
  int damage;
  char damage_msg[1024];

  hits = 0;
  if (strength <= 0)
    return -1;
  if (!ship->alive)
    return -1;

  if (ship->whatorbits != LEVEL_PLAN)
    return -1;

  ship_disposition(ship, &evade, &speed, &body);

  hits = Num_hits(0.0, 0, strength, Race->tech, 0, evade, 0, speed, 0, body,
                  MEDIUM, 1);

  damage = do_damage(Race->Playernum, ship, Race->tech, strength, hits, 0,
                     MEDIUM, 0.0, "medium guns", damage_msg);
  sprintf(short_msg, "%s [%d] %s %s\n", Dispshiploc(ship), Race->Playernum,
          ship->alive ? "attacked" : "DESTROYED", Ship(ship));
  strcpy(long_msg, short_msg);
  strcat(long_msg, damage_msg);

  return damage;
}
#endif

int shoot_ship_to_planet(shiptype *ship, planettype *pl, int strength, int x,
                         int y, int getmap, int ignore, int caliber,
                         char *long_msg, char *short_msg) {
  register sectortype *s, *target;
  register int x2, y2;
  int numdest, kills, oldowner;
  int i, num_sectors, sum_mob[MAXPLAYERS];
  double d, r, fac;

  numdest = 0;
  if (strength <= 0)
    return -1;
  if (!(ship->alive || ignore))
    return -1;
  if (has_switch(ship) && !ship->on)
    return -1;
  if (ship->whatorbits != LEVEL_PLAN)
    return -1;

  if (x < 0 || x > pl->Maxx - 1 || y < 0 || y > pl->Maxy - 1)
    return -1;

  r = .4 * strength;
  if (!caliber) { /* figure out the appropriate gun caliber if not given*/
    if (ship->fire_laser)
      caliber = LIGHT;
    else
      switch (ship->guns) {
      case PRIMARY:
        caliber = ship->primtype;
        break;
      case SECONDARY:
        caliber = ship->sectype;
        break;
      default:
        caliber = LIGHT;
      }
  }

  if (getmap) {
    getsmap(Smap, pl);
  }
  target = &Sector(*pl, x, y);
  oldowner = target->owner;

  for (i = 1; i <= Num_races; i++)
    sum_mob[i - 1] = 0;

  for (y2 = 0; y2 < pl->Maxy; y2++) {
    for (x2 = 0; x2 < pl->Maxx; x2++) {
      register int dx, dy;
      dx = MIN(abs(x2 - x), abs(x + (pl->Maxx - 1) - x2));
      dy = abs(y2 - y);
      d = sqrt((double)(dx * dx + dy * dy));
      s = &Sector(*pl, x2, y2);

      if (d <= r) {
        fac = SECTOR_DAMAGE * (double)strength * (double)caliber / (d + 1.);

        if (s->owner) {
          if (s->popn) {
            kills = int_rand(0, ((int)(fac / 10.0) * s->popn)) /
                    (1 + (s->condition == PLATED));
            if (kills > s->popn)
              s->popn = 0;
            else
              s->popn -= kills;
          }
          if (s->troops && (fac > 5.0 * (double)Defensedata[s->condition])) {
            kills = int_rand(0, ((int)(fac / 20.0) * s->troops)) /
                    (1 + (s->condition == PLATED));
            if (kills > s->troops)
              s->troops = 0;
            else
              s->troops -= kills;
          }

          if (!(s->popn + s->troops))
            s->owner = 0;
        }

        if (fac >= 5.0 && !int_rand(0, 10))
          mutate_sector(s);

        if (round_rand(fac) > Defensedata[s->condition] * int_rand(0, 10)) {
          if (s->owner)
            Nuked[s->owner - 1] = 1;
          s->popn = 0;
          s->troops = int_rand(0, (int)s->troops);
          if (!s->troops) /* troops may survive this */
            s->owner = 0;
          s->eff = 0;
          s->resource = s->resource / ((int)fac + 1);
          s->mobilization = 0;
          s->fert = 0; /*all is lost !*/
          s->crystals = int_rand(0, (int)s->crystals);
          s->condition = WASTED;
          numdest++;
        } else {
          s->fert = MAX(0, (int)s->fert - (int)fac);
          s->eff = MAX(0, (int)s->eff - (int)fac);
          s->mobilization = MAX(0, (int)s->mobilization - (int)fac);
          s->resource = MAX(0, (int)s->resource - (int)fac);
        }
      }
      if (s->owner)
        sum_mob[s->owner - 1] += s->mobilization;
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

  sprintf(short_msg, "%s bombards %s [%d]\n", Ship(ship), Dispshiploc(ship),
          oldowner);
  strcpy(long_msg, short_msg);
  sprintf(buf, "\t%d sectors destroyed\n", numdest);
  strcat(long_msg, buf);
  if (getmap) {
    putsmap(Smap, pl);
  }
  return numdest;
}

int do_radiation(shiptype *ship, double tech, int strength, int hits,
                 char *weapon, char *msg) {
  double fac, r;
  int i, arm, body, penetrate, casualties, casualties1;
  int dosage;

  fac =
      (2. / 3.14159265) * atan((double)(5 * (tech + 1.0) / (ship->tech + 1.0)));

  arm = MAX(0, Armor(ship) - hits / 5);
  body = Body(ship);

  penetrate = 0;
  r = 1.0;
  for (i = 1; i <= arm; i++)
    r *= fac;

  for (i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r)
      penetrate += 1;

  dosage = round_rand(40. * (double)penetrate / (double)body);
  dosage = MIN(100, dosage);

  if (dosage > ship->rad)
    ship->rad = MAX(ship->rad, dosage);
  if (success(ship->rad))
    ship->active = 0;

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

int do_damage(int who, shiptype *ship, double tech, int strength, int hits,
              int defense, int caliber, double range, char *weapon, char *msg) {
  double body;
  int i, arm;
  int damage;
  int penetrate, crithits, critdam;
  int casualties, casualties1, primgundamage, secgundamage;
  double fac, r;
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
  arm = MAX(0, Armor(ship) + defense - hits / 5);
  body = sqrt((double)(0.1 * Body(ship)));

  critdam = 0;
  crithits = 0;
  penetrate = 0;
  r = 1.0;
  for (i = 1; i <= arm; i++)
    r *= fac;

  for (i = 1; i <= hits; i++) /* check to see how many hits penetrate */
    if (double_rand() <= r)
      penetrate += 1;

  damage = round_rand(SHIP_DAMAGE * (double)caliber * (double)penetrate /
                      (double)body);

  do_critical_hits(penetrate, ship, &crithits, &critdam, caliber, critmsg);

  if (crithits)
    damage += critdam;

  damage = MIN(100, damage);
  ship->damage = MIN(100, (int)(ship->damage) + damage);
  do_collateral(ship, damage, &casualties, &casualties1, &primgundamage,
                &secgundamage);
  /* set laser strength for ships to maximum safe limit */
  if (ship->fire_laser) {
    int safe;
    safe = (int)((1.0 - .01 * ship->damage) * ship->tech / 4.0);
    if (ship->fire_laser > safe)
      ship->fire_laser = safe;
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

  if (ship->damage >= 100)
    kill_ship(who, ship);
  ship->build_cost = (int)cost(ship);
  return damage;
}

void ship_disposition(shiptype *ship, int *evade, int *speed, int *body) {
  *evade = 0;
  *speed = 0;
  *body = Size(ship);
  if (ship->active && !ship->docked && (ship->whatdest || ship->navigate.on)) {
    *evade = ship->protect.evade;
    *speed = ship->speed;
  }
  return;
}

int CEW_hit(double dist, int cew_range) {
  int prob, hits;

  hits = 0;
  prob = cew_hit_odds(dist, cew_range);

  if (success(prob))
    hits = 1;

  return hits;
}

int Num_hits(double dist, int focus, int guns, double tech, int fdam, int fev,
             int tev, int fspeed, int tspeed, int body, int caliber,
             int defense) {
  int factor;
  int i, prob, hits;

  prob = hit_odds(dist, &factor, tech, fdam, fev, tev, fspeed, tspeed, body,
                  caliber, defense);

  hits = 0;
  if (focus) {
    if (success(prob * prob / 100))
      hits = guns;
    hit_probability = prob * prob / 100;
  } else {
    for (i = 1; i <= guns; i++)
      if (success(prob))
        hits++;
    hit_probability = prob; /* global variable */
  }

  return hits;
}

int hit_odds(double range, int *factor, double tech, int fdam, int fev, int tev,
             int fspeed, int tspeed, int body, int caliber, int defense) {
  int odds;
  double a, b, c;

  if (caliber == NONE) {
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

int cew_hit_odds(double range, int cew_range) {
  int odds;
  double factor;

  factor = (range + 1.0) / ((double)cew_range + 1.0); /* maximum chance */
  odds = (int)(100.0 * exp((double)(-50.0 * (factor - 1.0) * (factor - 1.0))));
  return odds;
}

/*
 * gun range of given ship, given race and ship
 */
double gun_range(racetype *r, shiptype *s, int mode) {
  if (mode)
    return (logscale((int)(r->tech + 1.0)) * SYSTEMSIZE);
  else
    return (logscale((int)(s->tech + 1.0)) * SYSTEMSIZE);
}

/*
 * range of telescopes, ground or space, given race and ship
*/
double tele_range(int type, double tech) {
  if (type == OTYPE_GTELE)
    return log1p((double)tech) * 400 + SYSTEMSIZE / 8;
  else
    return log1p((double)tech) * 1500 + SYSTEMSIZE / 3;
}

int current_caliber(shiptype *ship) {
  if (ship->laser && ship->fire_laser)
    return LIGHT;
  else if (ship->type == STYPE_MINE)
    return LIGHT;
  else if (ship->type == STYPE_MISSILE)
    return HEAVY;
  else if (ship->guns == PRIMARY)
    return ship->primtype;
  else if (ship->guns == SECONDARY)
    return ship->sectype;
  else
    return NONE;
}

void do_critical_hits(int penetrate, shiptype *ship, int *crithits,
                      int *critdam, int caliber, char *critmsg) {
  int eff_size, i, dam;
  *critdam = 0;
  *crithits = 0;
  eff_size = MAX(1, Body(ship) / caliber);
  for (i = 1; i <= penetrate; i++)
    if (!int_rand(0, eff_size - 1)) {
      *crithits += 1;
      dam = int_rand(0, 100);
      *critdam += dam;
    }
  *critdam = MIN(100, *critdam);
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

void do_collateral(shiptype *ship, int damage, int *casualties,
                   int *casualties1, int *primgundamage, int *secgundamage) {
  int i;
  /* compute crew/troop casualties */
  *casualties = 0;
  *casualties1 = 0;
  *primgundamage = 0;
  *secgundamage = 0;

  for (i = 1; i <= ship->popn; i++)
    *casualties += success(damage);
  ship->popn -= *casualties;
  for (i = 1; i <= ship->troops; i++)
    *casualties1 += success(damage);
  ship->troops -= *casualties1;
  for (i = 1; i <= ship->primary; i++)
    *primgundamage += success(damage);
  ship->primary -= *primgundamage;
  for (i = 1; i <= ship->secondary; i++)
    *secgundamage += success(damage);
  ship->secondary -= *secgundamage;
  if (!ship->primary)
    ship->primtype = NONE;
  if (!ship->secondary)
    ship->sectype = NONE;
}

int getdefense(shiptype *ship) {
  planettype *p;
  sectortype *sect;
  int defense = 0;

  if (landed(ship)) {
    getplanet(&p, (int)ship->storbits, (int)ship->pnumorbits);
    getsector(&sect, p, (int)ship->land_x, (int)ship->land_y);
    defense = 2 * Defensedata[sect->condition];
    free(p);
    free(sect);
  }
  return defense;
}

double p_factor(double attacker, double defender) {
  return ((2. / 3.141592) *
          atan(5 * (double)((attacker + 1.0) / (defender + 1.0))));
}

int planet_guns(int points) {
  if (points < 0)
    return 0; /* shouldn't happen */
  return MIN(20, points / 1000);
}

void mutate_sector(sectortype *s) {
  if (int_rand(0, 6) >= Defensedata[s->condition])
    s->condition = s->type;
}
