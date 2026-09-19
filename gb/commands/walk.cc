// SPDX-License-Identifier: Apache-2.0

/// \file walk.cc
/// \brief Walk command implementation.

module;

import session;
import gb.entities;
import gb.services;
import notification;
import std;
#undef stdout

module commands;

namespace {

/**
 * @brief Validate AFV preconditions and Star AP before executing a walk move.
 */
const Ship* validate_walk_afv(GameObj& g, std::string_view ship_arg,
                              ap_t ap_cost) {
  auto shipno = string_to_shipnum(ship_arg);
  if (!shipno || *shipno <= 0) {
    g.out << "Bad ship number.\n";
    return nullptr;
  }
  const Ship* ship_peek = nullptr;
  try {
    ship_peek = g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "No such ship.\n";
    return nullptr;
  }
  if (!g.check_commandable(*ship_peek)) {
    g.out << "You do not control this ship.\n";
    return nullptr;
  }
  if (ship_peek->type() != ShipType::OTYPE_AFV) {
    g.out << "This ship doesn't walk!\n";
    return nullptr;
  }
  if (!ship_peek->is_landed()) {
    g.out << "This ship is not landed on a planet.\n";
    return nullptr;
  }
  if (!ship_peek->popn()) {
    g.out << "No crew.\n";
    return nullptr;
  }
  if (ship_peek->fuel() < AFV_FUEL_COST) {
    g.out << std::format("You don't have {:.1f} fuel to move it.\n",
                         AFV_FUEL_COST);
    return nullptr;
  }
  const auto& star = *g.entity_manager.peek_star(ship_peek->storbits());
  if (star.AP(g.player()) < ap_cost) {
    g.out << std::format("You don't have {} action points there.\n", ap_cost);
    return nullptr;
  }
  return ship_peek;
}

/**
 * @brief Compute and validate destination coordinates and sector compatibility
 * for an AFV walk move.
 */
std::optional<Coordinates> resolve_walk_destination(GameObj& g,
                                                    const Planet& planet,
                                                    const SectorMap& smap,
                                                    Coordinates old_coords,
                                                    char direction) {
  const Coordinates new_coords = get_move(planet, direction, old_coords);
  if (old_coords == new_coords) {
    g.out << "Illegal move.\n";
    return std::nullopt;
  }
  if (!planet.is_valid(new_coords)) {
    g.out << std::format("Illegal coordinates {}.\n", new_coords);
    return std::nullopt;
  }
  const auto& sect_check = smap.get(new_coords);
  if (!g.race->tolerates_sector(sect_check)) {
    g.out << "Your ships cannot walk into that sector type!\n";
    return std::nullopt;
  }
  return new_coords;
}

/**
 * @brief Execute a mutual gun duel between the moving AFV and a defending AFV
 * until one loses its retaliation strength or is destroyed.
 */
void resolve_afv_duel(GameObj& g, Ship& attacker, Ship& defender) {
  weapon_power_t def_strength = 0;
  weapon_power_t att_strength = 0;
  while ((def_strength = defender.retal_strength()) &&
         (att_strength = attacker.retal_strength())) {
    Ship attacker_snapshot(attacker.get_struct());
    if (auto def_shot = shoot_ship_to_ship(g.entity_manager, defender, attacker,
                                           def_strength, 0, false)) {
      const auto& [dmg1, short_buf, long_buf] = *def_shot;
      defender.consume_destruct(def_strength);
      g.out << long_buf;
      warn_player(g.session_registry, g.entity_manager, defender.owner(),
                  defender.governor(), long_buf);
      if (!attacker.alive()) {
        post(g.entity_manager, short_buf, NewsType::COMBAT);
      }
      notify_star(g.session_registry, g.entity_manager, g.player(),
                  g.governor(), attacker.storbits(), short_buf);
    }

    if (att_strength) {
      if (auto att_shot =
              shoot_ship_to_ship(g.entity_manager, attacker_snapshot, defender,
                                 att_strength, 0, true)) {
        const auto& [dmg2, short_buf2, long_buf2] = *att_shot;
        attacker.consume_destruct(att_strength);
        g.out << long_buf2;
        warn_player(g.session_registry, g.entity_manager, defender.owner(),
                    defender.governor(), long_buf2);
        if (!defender.alive()) {
          post(g.entity_manager, short_buf2, NewsType::COMBAT);
        }
        notify_star(g.session_registry, g.entity_manager, g.player(),
                    g.governor(), attacker.storbits(), short_buf2);
      }
    }
  }
}

/**
 * @brief Engage any non-allied landed AFVs occupying the destination sector.
 */
void engage_defending_afvs(GameObj& g, Ship& ship, starnum_t snum,
                           planetnum_t pnum, Coordinates new_coords) {
  const player_t playernum = g.player();
  for (auto ship_handle : ShipList::on_planet(g.entity_manager, snum, pnum)) {
    Ship& ship2 = *ship_handle;
    if (ship2.owner() != playernum && ship2.type() == ShipType::OTYPE_AFV &&
        ship2.is_landed() && ship2.retal_strength() &&
        ship2.land_coords() == new_coords) {
      const auto* alien = g.entity_manager.peek_race(ship2.owner());
      if (!alien) {
        continue;
      }
      if (!g.race->is_allied_with(ship2.owner()) ||
          !alien->is_allied_with(playernum)) {
        resolve_afv_duel(g, ship, ship2);
      }
    }
    if (!ship.alive()) {
      break;
    }
  }
}

/**
 * @brief Engage non-allied civilian and military population occupying the
 * destination sector.
 */
void engage_sector_defenders(GameObj& g, Ship& ship, const Star& star,
                             Planet& planet, Sector& sect,
                             Coordinates new_coords) {
  const player_t playernum = g.player();
  if (!ship.popn() || !ship.alive() || sect.get_owner() == 0 ||
      sect.get_owner() == playernum) {
    return;
  }

  const auto oldowner = sect.get_owner();
  const auto oldgov = star.governor(oldowner);
  const auto* alien = g.entity_manager.peek_race(oldowner);
  if (!alien ||
      (g.race->is_allied_with(oldowner) && alien->is_allied_with(playernum))) {
    return;
  }

  if (!ship.retal_strength()) {
    g.out << "You have nothing to attack with!\n";
    return;
  }

  while ((sect.get_popn() + sect.get_troops()) && ship.retal_strength()) {
    auto civ = sect.get_popn();
    auto mil = sect.get_troops();
    auto [short_buf, long_buf] = mech_attack_people(
        g.entity_manager, ship, &civ, &mil, *g.race, *alien, sect, false);
    g.out << long_buf;
    warn_player(g.session_registry, g.entity_manager, alien->Playernum, oldgov,
                long_buf);
    notify_star(g.session_registry, g.entity_manager, playernum, g.governor(),
                ship.storbits(), short_buf);
    post(g.entity_manager, short_buf, NewsType::COMBAT);

    auto [short_buf2, long_buf2] = people_attack_mech(
        g.entity_manager, ship, sect.get_popn(), sect.get_troops(), *alien,
        *g.race, sect, new_coords);
    g.out << long_buf2;
    warn_player(g.session_registry, g.entity_manager, alien->Playernum, oldgov,
                long_buf2);
    notify_star(g.session_registry, g.entity_manager, playernum, g.governor(),
                ship.storbits(), short_buf2);
    if (!ship.alive()) {
      post(g.entity_manager, short_buf2, NewsType::COMBAT);
    }

    sect.set_popn_exact(civ);
    sect.set_troops(mil);
    if (sect.is_empty()) {
      planet.info(oldowner).mob_points -=
          static_cast<int>(sect.get_mobilization());
      sect.set_owner(0);
    }
  }
}

/**
 * @brief Complete the AFV movement into the target sector if friendly or
 * cleared, deducting fuel and notifying other planetary inhabitants.
 */
void advance_afv_to_sector(GameObj& g, Ship& ship, const Star& star,
                           const Planet& planet, const Sector& sect,
                           Coordinates old_coords, Coordinates new_coords) {
  const player_t playernum = g.player();
  const bool can_occupy = sect.get_owner() == 0 ||
                          sect.get_owner() == playernum ||
                          g.race->is_allied_with(sect.get_owner());
  if (!ship.alive() || !ship.popn() || !can_occupy) {
    return;
  }

  std::string moving =
      std::format("{} moving from {} to {} on {}.\n", ship, old_coords,
                  new_coords, dispshiploc(g.entity_manager, ship));
  ship.set_land_coords(new_coords);
  ship.consume_fuel(AFV_FUEL_COST);
  for (player_t i{1}; i <= g.entity_manager.num_races();
       i = player_t{i.value + 1}) {
    if (i != playernum && planet.info(i).numsectsowned) {
      g.session_registry.notify_player(i, star.governor(i), moving);
    }
  }
}

}  // namespace

namespace GB::commands {

bool walk(const command_t& argv, GameObj& g) {
  const player_t playernum = g.player();
  const ap_t ap_cost = 1;

  const Ship* ship_peek = validate_walk_afv(g, argv[1], ap_cost);
  if (!ship_peek) {
    return false;
  }

  const shipnum_t shipno = ship_peek->number();
  const starnum_t snum = ship_peek->storbits();
  const planetnum_t pnum = ship_peek->pnumorbits();
  const Coordinates old_coords = ship_peek->land_coords();

  const auto& star = *g.entity_manager.peek_star(snum);
  const auto& planet_peek = *g.entity_manager.peek_planet(snum, pnum);
  const auto& smap_peek = *g.entity_manager.peek_sectormap(snum, pnum);

  auto new_coords_opt = resolve_walk_destination(g, planet_peek, smap_peek,
                                                 old_coords, argv[2][0]);
  if (!new_coords_opt) {
    return false;
  }
  const Coordinates new_coords = *new_coords_opt;

  g.entity_manager.mutate_ship(shipno, [&](Ship& ship) {
    engage_defending_afvs(g, ship, snum, pnum, new_coords);

    g.entity_manager.mutate_planet_and_sectors(
        snum, pnum, [&](Planet& planet, SectorMap& smap) {
          auto& sect = smap.get(new_coords);
          engage_sector_defenders(g, ship, star, planet, sect, new_coords);
          planet.sync_demographics(smap);
          advance_afv_to_sector(g, ship, star, planet, sect, old_coords,
                                new_coords);
        });
  });

  g.entity_manager.mutate_star(
      snum, [&](Star& star_mut) { star_mut.AP(playernum) -= ap_cost; });
  return true;
}

const CommandDescriptor walk_cmd{
    .name = "walk",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "walk <ship> <direction>",
    .description = "Move an AFV from one sector to another",
    .handler = &walk,
};

}  // namespace GB::commands
