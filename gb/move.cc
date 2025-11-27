// SPDX-License-Identifier: Apache-2.0

module;

/*  move.c -- move population and assault aliens on target sector */

import std.compat;

module gblib;

/**
 * @brief Calculates the new coordinates based on the given direction.
 *
 * This function takes a Planet object, a direction character, and the current
 * coordinates as input. It calculates and returns the new coordinates based on
 * the given direction.
 *
 * @param planet The Planet object representing the game world.
 * @param direction The direction character indicating the movement direction.
 * @param from The current coordinates.
 * @return The new coordinates after the movement.
 */
Coordinates get_move(const Planet& planet, const char direction,
                     const Coordinates from) {
  switch (direction) {
    case '1':
    case 'b': {
      Coordinates to{from.x - 1, from.y + 1};
      if (to.x == -1) to.x = planet.Maxx() - 1;
      return to;
    }
    case '2':
    case 'k':
      return Coordinates{from.x, from.y + 1};
    case '3':
    case 'n': {
      Coordinates to{from.x + 1, from.y + 1};
      if (to.x == planet.Maxx()) to.x = 0;
      return to;
    }
    case '4':
    case 'h': {
      Coordinates to{from.x - 1, from.y};
      if (to.x == -1) to.x = planet.Maxx() - 1;
      return to;
    }
    case '6':
    case 'l': {
      Coordinates to{from.x + 1, from.y};
      if (to.x == planet.Maxx()) to.x = 0;
      return to;
    }
    case '7':
    case 'y': {
      Coordinates to{from.x - 1, from.y - 1};
      if (to.x == -1) to.x = planet.Maxx() - 1;
      return to;
    }
    case '8':
    case 'j': {
      Coordinates to{from.x, from.y - 1};
      return to;
    }
    case '9':
    case 'u': {
      Coordinates to{from.x + 1, from.y - 1};
      if (to.x == planet.Maxx()) to.x = 0;
      return to;
    }
    default:
      return {from.x, from.y};
  }
}

void mech_defend(EntityManager& em, player_t Playernum, governor_t Governor,
                 int* people, PopulationType type, const Planet& p, int x2,
                 int y2, const Sector& s2) {
  population_t civ = 0;
  population_t mil = 0;
  int oldgov;

  if (type == PopulationType::CIV)
    civ = *people;
  else
    mil = *people;

  auto& race = races[Playernum - 1];

  ShipList shiplist(em, p.ships());
  for (auto ship_handle : shiplist) {
    if (civ + mil == 0) break;
    Ship& ship = *ship_handle;
    if (ship.owner != Playernum && ship.type == ShipType::OTYPE_AFV &&
        landed(ship) && retal_strength(ship) && (ship.land_x == x2) &&
        (ship.land_y == y2)) {
      auto& alien = races[ship.owner - 1];
      if (!isset(race.allied, ship.owner) || !isset(alien.allied, Playernum)) {
        while ((civ + mil) > 0 && retal_strength(ship)) {
          oldgov = stars[ship.storbits].governor(alien.Playernum - 1);
          char long_buf[1024], short_buf[256];
          mech_attack_people(ship, &civ, &mil, alien, race, s2, true, long_buf,
                             short_buf);
          notify(Playernum, Governor, long_buf);
          warn(alien.Playernum, oldgov, long_buf);
          if (civ + mil) {
            people_attack_mech(ship, civ, mil, race, alien, s2, x2, y2,
                               long_buf, short_buf);
            notify(Playernum, Governor, long_buf);
            warn(alien.Playernum, oldgov, long_buf);
          }
        }
      }
    }
  }
  *people = civ + mil;
}

void mech_attack_people(Ship& ship, population_t* civ, population_t* mil,
                        Race& race, Race& alien, const Sector& sect,
                        bool ignore, char* long_msg, char* short_msg) {
  auto oldciv = *civ;
  auto oldmil = *mil;

  auto strength = retal_strength(ship);
  auto astrength = MECH_ATTACK * ship.tech * (double)strength *
                   ((double)ship.armor + 1.0) * .01 *
                   (100.0 - (double)ship.damage) * .01 *
                   (race.likes[sect.get_condition()] + 1.0) *
                   morale_factor((double)(race.morale - alien.morale));

  auto dstrength = (double)(10 * oldmil * alien.fighters + oldciv) * 0.01 *
                   alien.tech * .01 *
                   (alien.likes[sect.get_condition()] + 1.0) *
                   ((double)Defensedata[sect.get_condition()] + 1.0) *
                   morale_factor((double)(alien.morale - race.morale));

  if (ignore) {
    auto ammo = static_cast<int>(std::log10(dstrength + 1.0)) - 1;
    ammo = std::min(std::max(ammo, 0), strength);
    use_destruct(ship, ammo);
  } else {
    use_destruct(ship, strength);
  }

  auto cas_civ =
      int_rand(0, round_rand((double)oldciv * astrength / dstrength));
  cas_civ = MIN(oldciv, cas_civ);
  auto cas_mil =
      int_rand(0, round_rand((double)oldmil * astrength / dstrength));
  cas_mil = MIN(oldmil, cas_mil);
  *civ -= cas_civ;
  *mil -= cas_mil;
  sprintf(short_msg, "%s: %s %s %s [%d]\n", dispshiploc(ship).c_str(),
          ship_to_string(ship).c_str(),
          (*civ + *mil) ? "attacked" : "slaughtered", alien.name.c_str(),
          alien.Playernum);
  strcpy(long_msg, short_msg);
  std::string battle_msg = std::format(
      "\tBattle at {},{} {}: {} guns fired on {} civ/{} mil\n", sect.get_x(),
      sect.get_y(), Desnames[sect.get_condition()], strength, oldciv, oldmil);
  strcat(long_msg, battle_msg.c_str());
  std::string attack_msg = std::format("\tAttack: {:.3f}   Defense: {:.3f}.\n",
                                       astrength, dstrength);
  strcat(long_msg, attack_msg.c_str());
  std::string casualties_msg =
      std::format("\t{} civ/{} mil killed.\n", cas_civ, cas_mil);
  strcat(long_msg, casualties_msg.c_str());
}

void people_attack_mech(Ship& ship, int civ, int mil, Race& race, Race& alien,
                        const Sector& sect, int x, int y, char* long_msg,
                        char* short_msg) {
  int strength;
  double astrength;
  double dstrength;
  int damage;
  int ammo;

  strength = retal_strength(ship);

  dstrength = MECH_ATTACK * ship.tech * (double)strength *
              ((double)ship.armor + 1.0) * .01 * (100.0 - (double)ship.damage) *
              .01 * (alien.likes[sect.get_condition()] + 1.0) *
              morale_factor((double)(alien.morale - race.morale));

  astrength = (double)(10 * mil * race.fighters + civ) * .01 * race.tech * .01 *
              (race.likes[sect.get_condition()] + 1.0) *
              ((double)Defensedata[sect.get_condition()] + 1.0) *
              morale_factor((double)(race.morale - alien.morale));
  ammo = (int)log10((double)astrength + 1.0) - 1;
  ammo = std::min(strength, std::max(0, ammo));
  use_destruct(ship, ammo);
  damage = int_rand(0, round_rand(100.0 * astrength / dstrength));
  damage = std::min(100, damage);
  ship.damage += damage;
  if (ship.damage >= 100) {
    ship.damage = 100;
    kill_ship(race.Playernum, &ship);
  }
  auto [cas_civ, cas_mil, pdam, sdam] = do_collateral(ship, damage);
  sprintf(short_msg, "%s: %s [%d] %s %s\n", dispshiploc(ship).c_str(),
          race.name.c_str(), race.Playernum,
          ship.alive ? "attacked" : "DESTROYED", ship_to_string(ship).c_str());
  strcpy(long_msg, short_msg);
  std::string assault_msg = std::format(
      "\tBattle at {},{} {}: {} civ/{} mil assault {}\n", x, y,
      Desnames[sect.get_condition()], civ, mil, Shipnames[ship.type]);
  strcat(long_msg, assault_msg.c_str());
  std::string attack_msg = std::format("\tAttack: {:.3f}   Defense: {:.3f}.\n",
                                       astrength, dstrength);
  strcat(long_msg, attack_msg.c_str());
  std::string damage_msg = std::format(
      "\t{}% damage inflicted for a total of {}%\n", damage, ship.damage);
  strcat(long_msg, damage_msg.c_str());
  std::string casualties_msg =
      std::format("\t{} civ/{} mil killed   {} prim/{} sec guns knocked out\n",
                  cas_civ, cas_mil, pdam, sdam);
  strcat(long_msg, casualties_msg.c_str());
}

void ground_attack(Race& race, Race& alien, int* people, PopulationType what,
                   population_t* civ, population_t* mil, unsigned int def1,
                   unsigned int def2, double alikes, double dlikes,
                   double* astrength, double* dstrength, int* casualties,
                   int* casualties2, int* casualties3) {
  int casualty_scale;

  *astrength = (double)(*people * race.fighters *
                        (what == PopulationType::MIL ? 10 : 1)) *
               (alikes + 1.0) * ((double)def1 + 1.0) *
               morale_factor((double)(race.morale - alien.morale));
  *dstrength = (double)((*civ + *mil * 10) * alien.fighters) * (dlikes + 1.0) *
               ((double)def2 + 1.0) *
               morale_factor((double)(alien.morale - race.morale));
  /* nuke both populations */
  casualty_scale =
      MIN(*people * (what == PopulationType::MIL ? 10 : 1) * race.fighters,
          (*civ + *mil * 10) * alien.fighters);

  *casualties =
      int_rand(0, round_rand((double)((casualty_scale /
                                       (what == PopulationType::MIL ? 10 : 1)) *
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
