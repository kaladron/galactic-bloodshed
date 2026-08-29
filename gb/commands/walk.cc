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

namespace GB::commands {
bool walk(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  const ap_t APcount = 1;

  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || *shipno <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }
  const Ship* ship_peek = nullptr;
  try {
    ship_peek = g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "No such ship.\n";
    return false;
  }
  if (testship(*ship_peek, g)) {
    g.out << "You do not control this ship.\n";
    return false;
  }
  if (ship_peek->type() != ShipType::OTYPE_AFV) {
    g.out << "This ship doesn't walk!\n";
    return false;
  }
  if (!ship_peek->is_landed()) {
    g.out << "This ship is not landed on a planet.\n";
    return false;
  }
  if (!ship_peek->popn()) {
    g.out << "No crew.\n";
    return false;
  }
  if (ship_peek->fuel() < AFV_FUEL_COST) {
    g.out << std::format("You don't have {:.1f} fuel to move it.\n",
                         AFV_FUEL_COST);
    return false;
  }

  const starnum_t snum = ship_peek->storbits();
  const planetnum_t pnum = ship_peek->pnumorbits();
  const auto& star = *g.entity_manager.peek_star(snum);

  if (star.AP(Playernum) < APcount) {
    g.out << std::format("You don't have {} action points there.\n", APcount);
    return false;
  }
  const auto& p = *g.entity_manager.peek_planet(snum, pnum);

  Coordinates old_coords = ship_peek->land_coords();
  Coordinates new_coords = get_move(p, argv[2][0], old_coords);
  if (old_coords == new_coords) {
    g.out << "Illegal move.\n";
    return false;
  }
  if (!p.is_valid(new_coords)) {
    g.out << std::format("Illegal coordinates {}.\n", new_coords);
    return false;
  }
  const auto& smap_peek = *g.entity_manager.peek_sectormap(snum, pnum);
  /* check to see if player is permited on the sector type */
  const auto& sect_check = smap_peek.get(new_coords);
  if (!g.race->likes[sect_check.get_condition()]) {
    g.out << "Your ships cannot walk into that sector type!\n";
    return false;
  }
  /* if the sector is occupied by non-aligned AFVs, each one will attack */
  g.entity_manager.mutate_ship(*shipno, [&](Ship& ship) {
    ShipList shiplist(g.entity_manager, p.ships());
    for (auto ship_handle : shiplist) {
      Ship& ship2 = *ship_handle;
      if (ship2.owner() != Playernum && ship2.type() == ShipType::OTYPE_AFV &&
          ship2.is_landed() && retal_strength(ship2) &&
          (ship2.land_coords() == new_coords)) {
        const auto* alien = g.entity_manager.peek_race(ship2.owner());
        if (!alien) {
          continue;
        }
        if (!isset(g.race->allied, ship2.owner()) ||
            !isset(alien->allied, Playernum)) {
          int strength;
          int strength1;
          while ((strength = retal_strength(ship2)) &&
                 (strength1 = retal_strength(ship))) {
            use_destruct(ship2, strength);
            std::string short_msg =
                std::format("{} AFV #{} attacked by AFV #{}\n",
                            dispshiploc(g.entity_manager, ship2),
                            ship2.number(), ship.number());
            std::string long_msg =
                short_msg + std::format("\t{} fired guns on AFV #{}\n",
                                        ship.number(), ship2.number());
            g.out << long_msg;
            warn_player(g.session_registry, g.entity_manager, ship2.owner(),
                        ship2.governor(), long_msg);
            if (!ship2.alive())
              post(g.entity_manager, short_msg, NewsType::COMBAT);
            notify_star(g.session_registry, g.entity_manager, Playernum,
                        Governor, ship.storbits(), short_msg);
            if (strength1) {
              use_destruct(ship, strength1);
              std::string short_msg2 =
                  std::format("{} AFV #{} retaliated against AFV #{}\n",
                              dispshiploc(g.entity_manager, ship),
                              ship.number(), ship2.number());
              std::string long_msg2 =
                  short_msg2 + std::format("\t{} fired guns on AFV #{}\n",
                                           ship2.number(), ship.number());
              g.out << long_msg2;
              warn_player(g.session_registry, g.entity_manager, ship2.owner(),
                          ship2.governor(), long_msg2);
              if (!ship2.alive())
                post(g.entity_manager, short_msg2, NewsType::COMBAT);
              notify_star(g.session_registry, g.entity_manager, Playernum,
                          Governor, ship.storbits(), short_msg2);
            }
          }
        }
      }
      if (!ship.alive()) break;
    }

    g.entity_manager.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
      auto& sect = smap.get(new_coords);
      /* if the sector is occupied by non-aligned player, attack them first */
      if (ship.popn() && ship.alive() && sect.get_owner() != 0 &&
          sect.get_owner() != Playernum) {
        auto oldowner = sect.get_owner();
        auto oldgov = star.governor(sect.get_owner());
        const auto* alien = g.entity_manager.peek_race(oldowner);
        if (alien && (!isset(g.race->allied, oldowner) ||
                      !isset(alien->allied, Playernum))) {
          if (!retal_strength(ship)) {
            g.out << "You have nothing to attack with!\n";
          } else {
            while ((sect.get_popn() + sect.get_troops()) &&
                   retal_strength(ship)) {
              auto civ = sect.get_popn();
              auto mil = sect.get_troops();
              auto [short_buf, long_buf] =
                  mech_attack_people(g.entity_manager, ship, &civ, &mil,
                                     *g.race, *alien, sect, false);
              g.out << long_buf;
              warn_player(g.session_registry, g.entity_manager,
                          alien->Playernum, oldgov, long_buf);
              notify_star(g.session_registry, g.entity_manager, Playernum,
                          Governor, ship.storbits(), short_buf);
              post(g.entity_manager, short_buf, NewsType::COMBAT);

              auto [short_buf2, long_buf2] = people_attack_mech(
                  g.entity_manager, ship, sect.get_popn(), sect.get_troops(),
                  *alien, *g.race, sect, new_coords);
              g.out << long_buf2;
              warn_player(g.session_registry, g.entity_manager,
                          alien->Playernum, oldgov, long_buf2);
              notify_star(g.session_registry, g.entity_manager, Playernum,
                          Governor, ship.storbits(), short_buf2);
              if (!ship.alive())
                post(g.entity_manager, short_buf2, NewsType::COMBAT);

              sect.set_popn_exact(civ);
              sect.set_troops(mil);
              if (sect.is_empty()) {
                g.entity_manager.mutate_planet(snum, pnum, [&](Planet& p_mut) {
                  p_mut.info(sect.get_owner()).mob_points -=
                      (int)sect.get_mobilization();
                });
                sect.set_owner(0);
              }
            }
          }
        }
      }

      int succ = 0;
      if ((sect.get_owner() == Playernum ||
           isset(g.race->allied, sect.get_owner()) || sect.get_owner() == 0) &&
          ship.alive())
        succ = 1;

      if (ship.alive() && ship.popn() && succ) {
        std::string moving =
            std::format("{} moving from {} to {} on {}.\n", ship, old_coords,
                        new_coords, dispshiploc(g.entity_manager, ship));
        ship.set_land_coords(new_coords);
        use_fuel(ship, AFV_FUEL_COST);
        for (player_t i{1}; i <= g.entity_manager.num_races();
             i = player_t{i.value + 1})
          if (i != Playernum && p.info(i).numsectsowned)
            g.session_registry.notify_player(i, star.governor(i), moving);
      }
    });
  });

  g.entity_manager.mutate_star(
      snum, [&](Star& star_mut) { star_mut.AP(Playernum) -= APcount; });
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
