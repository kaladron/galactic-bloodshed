// SPDX-License-Identifier: Apache-2.0

/// \file give.cc
/// \brief Transfer ship ownership to a mutual ally.

module;

import session;
import gb.entities;
import gb.services;
import notification;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool give(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  player_t who = get_player(g.entity_manager, argv[1]);
  if (who.value == 0) {
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
          if (ship.owner() != Playernum || !ship.alive()) {
            DontOwnErr(g.entity_manager, Playernum, g.governor(), *shipno);
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
          if (ship.ships() != 0 && !race.God) {
            g.out << "You can't give away this ship, it has other ships "
                     "loaded on it.\n";
            return;
          }

          if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
            if (!g.deduct_univ_ap(5)) {
              g.out << "You don't have enough universe action points.\n";
              return;
            }
          } else {
            if (!g.deduct_ap(ship.storbits(), 5)) {
              g.out << "You don't have enough action points in that system.\n";
              return;
            }
          }

          ship.owner() = who;
          ship.governor() = 0; /* give to the leader */
          capture_stuff(ship, g);

          /* set inhabited/explored bits */
          switch (ship.whatorbits()) {
            case ScopeLevel::LEVEL_UNIV:
              break;
            case ScopeLevel::LEVEL_STAR: {
              g.entity_manager.mutate_star(ship.storbits(), [&](Star& star) {
                star.mark_explored_by(who);
              });
              break;
            }
            case ScopeLevel::LEVEL_PLAN: {
              g.entity_manager.mutate_star(ship.storbits(), [&](Star& star) {
                star.mark_explored_by(who);
              });

              g.entity_manager.mutate_planet(
                  ship.storbits(), ship.pnumorbits(),
                  [&](Planet& planet) { planet.info(who).explored = 1; });
              break;
            }
            default:
              g.out << "Something wrong with this ship's scope.\n";
              return;
          }

          g.out << "Owner changed.\n";
          std::string givemsg =
              std::format("{} [{}] gave you {} at {}.\n", race.name, Playernum,
                          ship, prin_ship_orbits(g.entity_manager, ship));
          warn_player(g.session_registry, g.entity_manager, who, 0, givemsg);

          if (!race.God) {
            std::string postmsg =
                std::format("{} [{}] gives {} [{}] a ship.\n", race.name,
                            Playernum, alien.name, who);
            post(g.entity_manager, postmsg, NewsType::TRANSFER);
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
