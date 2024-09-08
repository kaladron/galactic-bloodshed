// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  move.c -- move population and assault aliens on target sector */

import gblib;
import std.compat;

#include "gb/move.h"

#include <strings.h>

#include "gb/buffers.h"

int get_move(char direction, int x, int y, int *x2, int *y2,
             const Planet &planet) {
  switch (direction) {
    case '1':
    case 'b':
      *x2 = x - 1;
      *y2 = y + 1;
      if (*x2 == -1) *x2 = planet.Maxx - 1;
      return 1;
    case '2':
    case 'k':
      *x2 = x;
      *y2 = y + 1;
      return 1;
    case '3':
    case 'n':
      *x2 = x + 1;
      *y2 = y + 1;
      if (*x2 == planet.Maxx) *x2 = 0;
      return 1;
    case '4':
    case 'h':
      *x2 = x - 1;
      *y2 = y;
      if (*x2 == -1) *x2 = planet.Maxx - 1;
      return 1;
    case '6':
    case 'l':
      *x2 = x + 1;
      *y2 = y;
      if (*x2 == planet.Maxx) *x2 = 0;
      return 1;
    case '7':
    case 'y':
      *x2 = x - 1;
      *y2 = y - 1;
      if (*x2 == -1) *x2 = planet.Maxx - 1;
      return 1;
    case '8':
    case 'j':
      *x2 = x;
      *y2 = y - 1;
      return 1;
    case '9':
    case 'u':
      *x2 = x + 1;
      *y2 = y - 1;
      if (*x2 == planet.Maxx) *x2 = 0;
      return 1;
    default:
      *x2 = x;
      *y2 = y;
      return 0;
  }
}

void mech_defend(player_t Playernum, governor_t Governor, int *people, int type,
                 const Planet &p, int x2, int y2, const Sector &s2) {
  int civ = 0;
  int mil = 0;
  int oldgov;

  if (type == CIV)
    civ = *people;
  else
    mil = *people;

  auto &race = races[Playernum - 1];

  Shiplist shiplist{p.ships};
  for (auto ship : shiplist) {
    if (civ + mil == 0) break;
    if (ship.owner != Playernum && ship.type == ShipType::OTYPE_AFV &&
        landed(ship) && retal_strength(ship) && (ship.land_x == x2) &&
        (ship.land_y == y2)) {
      auto &alien = races[ship.owner - 1];
      if (!isset(race.allied, ship.owner) || !isset(alien.allied, Playernum)) {
        while ((civ + mil) > 0 && retal_strength(ship)) {
          oldgov = stars[ship.storbits].governor[alien.Playernum - 1];
          char long_buf[1024], short_buf[256];
          mech_attack_people(&ship, &civ, &mil, alien, race, s2, x2, y2, 1,
                             long_buf, short_buf);
          notify(Playernum, Governor, long_buf);
          warn(alien.Playernum, oldgov, long_buf);
          if (civ + mil) {
            people_attack_mech(&ship, civ, mil, race, alien, s2, x2, y2,
                               long_buf, short_buf);
            notify(Playernum, Governor, long_buf);
            warn(alien.Playernum, oldgov, long_buf);
          }
        }
      }
      putship(&ship);
    }
  }
  *people = civ + mil;
}

void mech_attack_people(Ship *ship, int *civ, int *mil, Race &race, Race &alien,
                        const Sector &sect, int x, int y, int ignore,
                        char *long_msg, char *short_msg) {
  int strength;
  int oldciv;
  int oldmil;
  double astrength;
  double dstrength;
  int cas_civ;
  int cas_mil;
  int ammo;

  oldciv = *civ;
  oldmil = *mil;

  strength = retal_strength(*ship);
  astrength = MECH_ATTACK * ship->tech * (double)strength *
              ((double)ship->armor + 1.0) * .01 *
              (100.0 - (double)ship->damage) * .01 *
              (race.likes[sect.condition] + 1.0) *
              morale_factor((double)(race.morale - alien.morale));

  dstrength = (double)(10 * oldmil * alien.fighters + oldciv) * 0.01 *
              alien.tech * .01 * (alien.likes[sect.condition] + 1.0) *
              ((double)Defensedata[sect.condition] + 1.0) *
              morale_factor((double)(alien.morale - race.morale));

  if (ignore) {
    ammo = (int)log10((double)dstrength + 1.0) - 1;
    ammo = std::min(std::max(ammo, 0), strength);
    use_destruct(*ship, ammo);
  } else
    use_destruct(*ship, strength);

  cas_civ = int_rand(0, round_rand((double)oldciv * astrength / dstrength));
  cas_civ = MIN(oldciv, cas_civ);
  cas_mil = int_rand(0, round_rand((double)oldmil * astrength / dstrength));
  cas_mil = MIN(oldmil, cas_mil);
  *civ -= cas_civ;
  *mil -= cas_mil;
  sprintf(short_msg, "%s: %s %s %s [%d]\n", dispshiploc(*ship).c_str(),
          ship_to_string(*ship).c_str(),
          (*civ + *mil) ? "attacked" : "slaughtered", alien.name,
          alien.Playernum);
  strcpy(long_msg, short_msg);
  sprintf(buf, "\tBattle at %d,%d %s: %d guns fired on %d civ/%d mil\n", x, y,
          Desnames[sect.condition], strength, oldciv, oldmil);
  strcat(long_msg, buf);
  sprintf(buf, "\tAttack: %.3f   Defense: %.3f.\n", astrength, dstrength);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d civ/%d mil killed.\n", cas_civ, cas_mil);
  strcat(long_msg, buf);
}

void people_attack_mech(Ship *ship, int civ, int mil, Race &race, Race &alien,
                        const Sector &sect, int x, int y, char *long_msg,
                        char *short_msg) {
  int strength;
  double astrength;
  double dstrength;
  int damage;
  int ammo;

  strength = retal_strength(*ship);

  dstrength = MECH_ATTACK * ship->tech * (double)strength *
              ((double)ship->armor + 1.0) * .01 *
              (100.0 - (double)ship->damage) * .01 *
              (alien.likes[sect.condition] + 1.0) *
              morale_factor((double)(alien.morale - race.morale));

  astrength = (double)(10 * mil * race.fighters + civ) * .01 * race.tech * .01 *
              (race.likes[sect.condition] + 1.0) *
              ((double)Defensedata[sect.condition] + 1.0) *
              morale_factor((double)(race.morale - alien.morale));
  ammo = (int)log10((double)astrength + 1.0) - 1;
  ammo = std::min(strength, std::max(0, ammo));
  use_destruct(*ship, ammo);
  damage = int_rand(0, round_rand(100.0 * astrength / dstrength));
  damage = std::min(100, damage);
  ship->damage += damage;
  if (ship->damage >= 100) {
    ship->damage = 100;
    kill_ship(race.Playernum, ship);
  }
  auto [cas_civ, cas_mil, pdam, sdam] = do_collateral(*ship, damage);
  sprintf(short_msg, "%s: %s [%d] %s %s\n", dispshiploc(*ship).c_str(),
          race.name, race.Playernum, ship->alive ? "attacked" : "DESTROYED",
          ship_to_string(*ship).c_str());
  strcpy(long_msg, short_msg);
  sprintf(buf, "\tBattle at %d,%d %s: %d civ/%d mil assault %s\n", x, y,
          Desnames[sect.condition], civ, mil, Shipnames[ship->type]);
  strcat(long_msg, buf);
  sprintf(buf, "\tAttack: %.3f   Defense: %.3f.\n", astrength, dstrength);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d%% damage inflicted for a total of %d%%\n", damage,
          ship->damage);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d civ/%d mil killed   %d prim/%d sec guns knocked out\n",
          cas_civ, cas_mil, pdam, sdam);
  strcat(long_msg, buf);
}

void ground_attack(Race &race, Race &alien, int *people, int what,
                   population_t *civ, population_t *mil, unsigned int def1,
                   unsigned int def2, double alikes, double dlikes,
                   double *astrength, double *dstrength, int *casualties,
                   int *casualties2, int *casualties3) {
  int casualty_scale;

  *astrength = (double)(*people * race.fighters * (what == MIL ? 10 : 1)) *
               (alikes + 1.0) * ((double)def1 + 1.0) *
               morale_factor((double)(race.morale - alien.morale));
  *dstrength = (double)((*civ + *mil * 10) * alien.fighters) * (dlikes + 1.0) *
               ((double)def2 + 1.0) *
               morale_factor((double)(alien.morale - race.morale));
  /* nuke both populations */
  casualty_scale = MIN(*people * (what == MIL ? 10 : 1) * race.fighters,
                       (*civ + *mil * 10) * alien.fighters);

  *casualties = int_rand(
      0, round_rand((double)((casualty_scale / (what == MIL ? 10 : 1)) *
                             *dstrength / *astrength)));
  *casualties = std::min(*people, *casualties);
  *people -= *casualties;

  *casualties2 =
      int_rand(0, round_rand((double)casualty_scale * *astrength / *dstrength));
  *casualties2 = MIN(*civ, *casualties2);
  *civ -= *casualties2;
  /* and for troops */
  *casualties3 = int_rand(
      0, round_rand((double)(casualty_scale / 10) * *astrength / *dstrength));
  *casualties3 = MIN(*mil, *casualties3);
  *mil -= *casualties3;
}
