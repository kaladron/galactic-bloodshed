// SPDX-License-Identifier: Apache-2.0

/// \file move.cc
/// \brief Move population and assault aliens on target sector.

module;

import std;

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
  Coordinates offset{0, 0};
  switch (direction) {
    case '1':
    case 'b':
      offset = {-1, 1};
      break;
    case '2':
    case 'k':
      offset = {0, 1};
      break;
    case '3':
    case 'n':
      offset = {1, 1};
      break;
    case '4':
    case 'h':
      offset = {-1, 0};
      break;
    case '6':
    case 'l':
      offset = {1, 0};
      break;
    case '7':
    case 'y':
      offset = {-1, -1};
      break;
    case '8':
    case 'j':
      offset = {0, -1};
      break;
    case '9':
    case 'u':
      offset = {1, -1};
      break;
    default:
      return from;
  }
  return planet.wrap(from + offset);
}

void mech_defend(const GameObj& g, population_t* people, PopulationType type,
                 const Planet& p, Coordinates target_coords, const Sector& s2) {
  population_t civ = 0;
  population_t mil = 0;
  governor_t oldgov;

  if (type == PopulationType::CIV)
    civ = *people;
  else
    mil = *people;

  ShipList shiplist(g.entity_manager, p.ships());
  for (auto ship_handle : shiplist) {
    if (civ + mil == 0) break;
    Ship& ship = *ship_handle;
    if (ship.owner() != g.player() && ship.type() == ShipType::OTYPE_AFV &&
        ship.is_landed() && retal_strength(ship) &&
        (ship.land_coords() == target_coords)) {
      const auto* alien_ptr = g.entity_manager.peek_race(ship.owner());
      if (!g.race->is_allied_with(ship.owner()) ||
          !alien_ptr->is_allied_with(g.player())) {
        const auto* star = g.entity_manager.peek_star(ship.storbits());
        while ((civ + mil) > 0 && retal_strength(ship)) {
          oldgov = star->governor(alien_ptr->Playernum);
          auto [short_buf, long_buf] =
              mech_attack_people(g.entity_manager, ship, &civ, &mil, *alien_ptr,
                                 *g.race, s2, true);
          push_telegram(g.entity_manager, g.player(), g.governor(), long_buf);
          push_telegram(g.entity_manager, alien_ptr->Playernum, oldgov,
                        long_buf);
          if (civ + mil) {
            auto [short_buf2, long_buf2] =
                people_attack_mech(g.entity_manager, ship, civ, mil, *g.race,
                                   *alien_ptr, s2, target_coords);
            push_telegram(g.entity_manager, g.player(), g.governor(),
                          long_buf2);
            push_telegram(g.entity_manager, alien_ptr->Playernum, oldgov,
                          long_buf2);
          }
        }
      }
    }
  }
  *people = civ + mil;
}

std::tuple<std::string, std::string>
mech_attack_people(EntityManager& em, Ship& ship, population_t* civ,
                   population_t* mil, const Race& race, const Race& alien,
                   const Sector& sect, bool ignore) {
  auto oldciv = *civ;
  auto oldmil = *mil;

  auto strength = retal_strength(ship);
  auto astrength = MECH_ATTACK * ship.tech() * (double)strength *
                   ((double)ship.armor() + 1.0) * .01 *
                   (100.0 - (double)ship.damage()) * .01 *
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
  std::string short_msg =
      std::format("{}: {} {} {} [{}]\n", dispshiploc(em, ship), ship,
                  (*civ + *mil) ? "attacked" : "slaughtered", alien.name,
                  alien.Playernum.value);
  std::string long_msg =
      short_msg +
      std::format("\tBattle at {},{} {}: {} guns fired on {} civ/{} mil\n"
                  "\tAttack: {:.3f}   Defense: {:.3f}.\n"
                  "\t{} civ/{} mil killed.\n",
                  sect.get_x(), sect.get_y(), Desnames[sect.get_condition()],
                  strength, oldciv, oldmil, astrength, dstrength, cas_civ,
                  cas_mil);
  return std::make_tuple(short_msg, long_msg);
}

std::tuple<std::string, std::string>
people_attack_mech(EntityManager& em, Ship& ship, int civ, int mil,
                   const Race& race, const Race& alien, const Sector& sect,
                   Coordinates target_coords) {
  int strength;
  double astrength;
  double dstrength;
  int damage;
  int ammo;

  strength = retal_strength(ship);

  dstrength = MECH_ATTACK * ship.tech() * (double)strength *
              ((double)ship.armor() + 1.0) * .01 *
              (100.0 - (double)ship.damage()) * .01 *
              (alien.likes[sect.get_condition()] + 1.0) *
              morale_factor((double)(alien.morale - race.morale));

  astrength = (double)(10 * mil * race.fighters + civ) * .01 * race.tech * .01 *
              (race.likes[sect.get_condition()] + 1.0) *
              ((double)Defensedata[sect.get_condition()] + 1.0) *
              morale_factor((double)(race.morale - alien.morale));
  ammo = (int)std::log10((double)astrength + 1.0) - 1;
  ammo = std::min(strength, std::max(0, ammo));
  use_destruct(ship, ammo);
  damage = int_rand(0, round_rand(100.0 * astrength / dstrength));
  damage = std::min(100, damage);
  ship.damage() += damage;
  if (ship.damage() >= 100) {
    ship.damage() = 100;
    em.kill_ship(race.Playernum, ship);
  }
  auto [cas_civ, cas_mil, pdam, sdam] = do_collateral(ship, damage);
  std::string short_msg = std::format(
      "{}: {} [{}] {} {}\n", dispshiploc(em, ship), race.name,
      race.Playernum.value, ship.alive() ? "attacked" : "DESTROYED", ship);
  std::string long_msg =
      short_msg +
      std::format("\tBattle at {} {}: {} civ/{} mil assault {}\n"
                  "\tAttack: {:.3f}   Defense: {:.3f}.\n"
                  "\t{}% damage inflicted for a total of {}%\n"
                  "\t{} civ/{} mil killed   {} prim/{} sec guns knocked out\n",
                  target_coords, Desnames[sect.get_condition()], civ, mil,
                  Shipnames[ship.type()], astrength, dstrength, damage,
                  ship.damage(), cas_civ, cas_mil, pdam, sdam);
  return std::make_tuple(short_msg, long_msg);
}

void ground_attack(const Race& race, const Race& alien, population_t* people,
                   PopulationType what, population_t* civ, population_t* mil,
                   unsigned int def1, unsigned int def2, double alikes,
                   double dlikes, double* astrength, double* dstrength,
                   population_t* casualties, population_t* casualties2,
                   population_t* casualties3) {
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
