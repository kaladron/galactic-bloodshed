// SPDX-License-Identifier: Apache-2.0

/// \file give.cc
/// \brief Transfer ship ownership to a mutual ally.

module;

import gb.entities;
import gb.presentation;
import gb.presentation.navigation;
import gb.services;
import std;

module commands;

namespace GB::commands {
bool give(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  player_t who = get_player(g.entity_manager, argv[1]);
  if (who == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }

  try {
    bool ok = false;
    g.entity_manager.with_race(who, [&](const Race& alien) {
      const auto& race = *g.race;
      if (alien.Guest && !race.God) {
        g.out << "You can't give this player anything.\n";
        return;
      }
      /* check to see if both players are mutually allied */
      if (!race.God &&
          !(race.is_allied_with(who) && alien.is_allied_with(Playernum))) {
        g.out << "You two are not mutually allied.\n";
        return;
      }
      auto shipno = string_to_shipnum(argv[2]);
      if (!shipno) {
        g.out << "Illegal ship number.\n";
        return;
      }

      try {
        g.entity_manager.mutate_ship(*shipno, [&](Ship& ship) {
          if (ship.owner() != Playernum) {
            g.out << std::format("You don't own ship #{}.\n", *shipno);
            return;
          }
          if (ship.type() == ShipType::STYPE_POD) {
            g.out << "You cannot change the ownership of spore pods.\n";
            return;
          }

          if ((ship.popn() + ship.troops()) && !race.God) {
            g.out << "You can't give this ship away while it has crew/mil on "
                     "board.\n";
            return;
          }
          if (!ShipList::readonly_in_carrier(g.entity_manager, ship.number())
                   .empty() &&
              !race.God) {
            g.out << "You can't give away this ship, it has other ships "
                     "loaded on it.\n";
            return;
          }

          if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
            if (!g.deduct_univ_ap(5)) {
              g.out << "You don't have enough universe action points.\n";
              return;
            }
          } else if (ship.whatorbits() == ScopeLevel::LEVEL_STAR ||
                     ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
            if (!g.deduct_ap(ship.storbits(), 5)) {
              g.out << "You don't have enough action points in that system.\n";
              return;
            }
          } else {
            g.out << "Something wrong with this ship's scope.\n";
            return;
          }

          ship.owner() = who;
          ship.governor() = Race::leader_id; /* give to the leader */
          g.present(capture_stuff(g.entity_manager, ship));

          /* set inhabited/explored bits */
          if (ship.whatorbits() == ScopeLevel::LEVEL_STAR ||
              ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
            g.entity_manager.mutate_star(ship.storbits(), [&](Star& star) {
              star.mark_explored_by(who);
            });
          }
          if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
            g.entity_manager.mutate_planet(
                ship.storbits(), ship.pnumorbits(),
                [&](Planet& planet) { planet.info(who).explored = 1; });
          }

          const auto result = presentation::make_give_result(
              ship, who, alien.name, prin_ship_orbits(g.entity_manager, ship));
          g.present(result);
          warn_player(
              g.session_registry, g.entity_manager, who, Race::leader_id,
              presentation::render_give_recipient_telegram(
                  race.name, Playernum, result.ship_desc, result.orbit_path));

          if (!race.God) {
            post(g.entity_manager,
                 presentation::render_give_news(race.name, Playernum,
                                                alien.name, who),
                 NewsType::TRANSFER);
          }
          ok = true;
        });
      } catch (const EntityNotFoundError&) {
        g.out << "No such ship.\n";
      }
    });
    return ok;
  } catch (const EntityNotFoundError&) {
    g.out << "Race not found.\n";
    return false;
  }
}

const CommandDescriptor give_cmd{
    .name = "give",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "give <race> <#ship>",
    .description = "Transfer ownership of an uncrewed ship to a mutual ally",
    .handler = &give,
};

}  // namespace GB::commands
