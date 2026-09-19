// SPDX-License-Identifier: Apache-2.0

/// \file move.cc
/// \brief Move population and assault aliens on target sector.

module;

import std;

module gblib;

namespace {

// TODO(C++26): Use std::inplace_vector when it lands in libc++ and make
// constexpr when P3372 (constexpr containers and adaptors) lands.
const std::flat_map<char, Coordinates> direction_mappings{
    {'1', {-1, 1}},  {'b', {-1, 1}},   // Southwest
    {'2', {0, 1}},   {'k', {0, 1}},    // South
    {'3', {1, 1}},   {'n', {1, 1}},    // Southeast
    {'4', {-1, 0}},  {'h', {-1, 0}},   // West
    {'6', {1, 0}},   {'l', {1, 0}},    // East
    {'7', {-1, -1}}, {'y', {-1, -1}},  // Northwest
    {'8', {0, -1}},  {'j', {0, -1}},   // North
    {'9', {1, -1}},  {'u', {1, -1}},   // Northeast
};

bool is_hostile_defending_afv(const GameObj& g, const Ship& ship,
                              Coordinates target_coords,
                              const Race& alien_race) {
  if (ship.owner() == g.player() || ship.type() != ShipType::OTYPE_AFV ||
      !ship.is_landed() || ship.retal_strength() == 0 ||
      ship.land_coords() != target_coords) {
    return false;
  }
  return !g.race->is_allied_with(ship.owner()) ||
         !alien_race.is_allied_with(g.player());
}

void resolve_afv_sector_engagement(const GameObj& g, Ship& ship,
                                   const Race& alien_race, governor_t alien_gov,
                                   const Sector& sect,
                                   Coordinates target_coords, population_t& civ,
                                   population_t& mil) {
  while ((civ + mil) > 0 && ship.retal_strength() > 0) {
    auto [short_buf, long_buf] = mech_attack_people(
        g.entity_manager, ship, &civ, &mil, alien_race, *g.race, sect, true);
    push_telegram(g.entity_manager, g.player(), g.governor(), long_buf);
    push_telegram(g.entity_manager, alien_race.Playernum, alien_gov, long_buf);
    if (civ + mil > 0) {
      auto [short_buf2, long_buf2] =
          people_attack_mech(g.entity_manager, ship, civ, mil, *g.race,
                             alien_race, sect, target_coords);
      push_telegram(g.entity_manager, g.player(), g.governor(), long_buf2);
      push_telegram(g.entity_manager, alien_race.Playernum, alien_gov,
                    long_buf2);
    }
  }
}

}  // namespace

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
  if (const auto it = direction_mappings.find(direction);
      it != direction_mappings.end()) {
    return planet.wrap(from + it->second);
  }
  return from;
}

void mech_defend(const GameObj& g, population_t* people, PopulationType type,
                 const Planet& p, Coordinates target_coords, const Sector& s2) {
  population_t civ = (type == PopulationType::CIV) ? *people : 0;
  population_t mil = (type == PopulationType::CIV) ? 0 : *people;

  for (auto ship_handle :
       ShipList::on_planet(g.entity_manager, p.star_id(), p.planet_order())) {
    if (civ + mil == 0) break;
    Ship& ship = *ship_handle;
    const auto* alien_ptr = g.entity_manager.peek_race(ship.owner());
    if (!alien_ptr ||
        !is_hostile_defending_afv(g, ship, target_coords, *alien_ptr)) {
      continue;
    }
    const auto* star = g.entity_manager.peek_star(ship.storbits());
    const governor_t oldgov = star->governor(alien_ptr->Playernum);
    resolve_afv_sector_engagement(g, ship, *alien_ptr, oldgov, s2,
                                  target_coords, civ, mil);
  }
  *people = civ + mil;
}

std::tuple<std::string, std::string>
mech_attack_people(EntityManager& em, Ship& ship, population_t* civ,
                   population_t* mil, const Race& race, const Race& alien,
                   const Sector& sect, bool ignore) {
  auto oldciv = *civ;
  auto oldmil = *mil;

  auto strength = ship.retal_strength();
  auto astrength = MECH_ATTACK * ship.tech() * (double)strength *
                   ((double)ship.armor() + 1.0) * .01 *
                   (100.0 - (double)ship.damage()) * .01 *
                   race.sector_combat_factor(sect) *
                   morale_factor((double)(race.morale - alien.morale));

  auto dstrength = (double)(10 * oldmil * alien.fighters + oldciv) * 0.01 *
                   alien.tech * .01 * alien.sector_combat_factor(sect) *
                   sect.combat_defense_factor() *
                   morale_factor((double)(alien.morale - race.morale));

  if (ignore) {
    auto raw_ammo = static_cast<int>(std::log10(dstrength + 1.0)) - 1;
    auto ammo =
        std::min(static_cast<weapon_power_t>(std::max(raw_ammo, 0)), strength);
    ship.consume_destruct(ammo);
  } else {
    ship.consume_destruct(strength);
  }

  const double ratio = (dstrength > 0.0) ? std::min(1e6, astrength / dstrength)
                                         : (astrength > 0.0 ? 1e6 : 0.0);
  auto cas_civ = int_rand(0, round_rand(static_cast<double>(oldciv) * ratio));
  cas_civ = MIN(oldciv, cas_civ);
  auto cas_mil = int_rand(0, round_rand(static_cast<double>(oldmil) * ratio));
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
  auto strength = ship.retal_strength();

  const double dstrength = MECH_ATTACK * ship.tech() * (double)strength *
                           ((double)ship.armor() + 1.0) * .01 *
                           (100.0 - (double)ship.damage()) * .01 *
                           alien.sector_combat_factor(sect) *
                           morale_factor((double)(alien.morale - race.morale));

  const double astrength = (double)(10 * mil * race.fighters + civ) * .01 *
                           race.tech * .01 * race.sector_combat_factor(sect) *
                           sect.combat_defense_factor() *
                           morale_factor((double)(race.morale - alien.morale));
  auto raw_ammo = (int)std::log10((double)astrength + 1.0) - 1;
  auto ammo =
      std::min(strength, static_cast<weapon_power_t>(std::max(0, raw_ammo)));
  ship.consume_destruct(ammo);
  const double damage_ceiling =
      (dstrength > 0.0) ? std::min(1e6, 100.0 * astrength / dstrength)
                        : (astrength > 0.0 ? 100.0 : 0.0);
  auto damage = int_rand(0, round_rand(damage_ceiling));
  damage = std::min(100, damage);
  if (ship.apply_damage(damage).destroyed) {
    em.kill_ship(race.Playernum, ship);
  }
  auto [cas_civ, cas_mil, pdam, sdam] = do_collateral(ship, damage, alien.mass);
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
                  ship.type_name(), astrength, dstrength, damage, ship.damage(),
                  cas_civ, cas_mil, pdam, sdam);
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
