// SPDX-License-Identifier: Apache-2.0

/// \file enslave.cc
/// \brief Enslave planet population.

module;

import std;
import gb.entities;
import gb.services;
import notification;
import session;

module commands;

namespace GB::commands {

bool enslave(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  ap_t APcount = 2;
  int aliens = 0;
  int def = 0;
  int attack = 0;

  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno) return false;
  try {
    bool ok = false;
    g.entity_manager.with_ship(*shipno, [&](const Ship& s) {
      if (testship(s, g)) {
        return;
      }
      if (s.type() != ShipType::STYPE_OAP) {
        g.out << std::format("This ship is not an {}.\n",
                             Shipnames[ShipType::STYPE_OAP]);
        return;
      }
      if (s.whatorbits() != ScopeLevel::LEVEL_PLAN) {
        g.out << std::format("{} doesn't orbit a planet.\n", s);
        return;
      }

      if (!g.deduct_ap(s.storbits(), APcount)) {
        g.out << "You don't have enough action points.\n";
        return;
      }

      std::string star_name;
      std::string planet_name;
      g.entity_manager.with_star(s.storbits(), [&](const Star& star) {
        star_name = star.get_name();
        planet_name = star.get_planet_name(s.pnumorbits());
      });

      g.entity_manager.mutate_planet(
          s.storbits(), s.pnumorbits(), [&](Planet& p) {
            if (p.info(Playernum).numsectsowned == 0) {
              g.out << "You don't have a garrison on the planet.\n";
              return;
            }

            /* add up forces attacking, defending */
            attack = aliens = def = 0;
            for (player_t i = 1; i < MAXPLAYERS; i++) {
              if (p.info(i).numsectsowned && i != Playernum) {
                aliens = 1;
                def += p.info(i).destruct;
              }
            }

            if (!aliens) {
              g.out << "There is no one else on this planet to enslave!\n";
              return;
            }

            const ShipList kShiplist(g.entity_manager, p.ships());
            for (const Ship& s2 : kShiplist) {
              if (s2.alive() && s2.active()) {
                if (p.info(s2.owner()).numsectsowned && s2.owner() != Playernum)
                  def += s2.destruct();
                else if (s2.owner() == Playernum)
                  attack += s2.destruct();
              }
            }

            g.out << "\nFor successful enslavement this ship and the other "
                     "ships here\n";
            g.out << "that are yours must have a weapons\n";
            g.out << "capacity greater than twice that the enemy can muster, "
                     "including\n";
            g.out << "the planet and all ships orbiting it.\n";
            g.out << std::format("\nTotal forces bearing on {}:   {}\n",
                                 prin_ship_orbits(g.entity_manager, s), attack);

            std::stringstream telegram;
            telegram << std::format("ALERT!!!\n\nPlanet /{}/{}", star_name,
                                    planet_name);

            if (def <= 2 * attack) {
              p.slaved_to() = Playernum;

              /* send telegs to anyone there */
              telegram << std::format("ENSLAVED by {}!!\n", s);
              telegram << std::format("All material produced here will be\n"
                                      "diverted to {} coffers.",
                                      g.race->name);

              g.out << "\nEnslavement successful.  All material produced here "
                       "will\n";
              g.out << std::format("be diverted to {}.\n", g.race->name);
              g.out << std::format("You must maintain a garrison of 0.1%% "
                                   "the population of the\n");
              g.out << std::format("planet (at least {:.0f}); otherwise there "
                                   "is a 50% chance that\n",
                                   p.popn() * 0.001);
              g.out << std::format("enslaved population will revolt.\n");
            } else {
              telegram << std::format(
                  "repulsed attempt at enslavement by {}!!\n", s);
              telegram << std::format("Enslavement repulsed, defense/attack "
                                      "Ratio : {} to {}.\n",
                                      def, attack);

              g.out << "Enslavement repulsed.\n";
              g.out << "You needed more weapons bearing on the planet...\n";
            }

            g.entity_manager.with_star(s.storbits(), [&](const Star& star) {
              for (player_t i{1}; i.value < MAXPLAYERS; ++i)
                if (p.info(i).numsectsowned && i != Playernum)
                  warn_player(g.session_registry, g.entity_manager, i,
                              star.governor(i), telegram.str());
            });

            ok = true;
          });
    });
    return ok;
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return false;
  }
}

const CommandDescriptor enslave_cmd{
    .name = "enslave",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "enslave <ship>",
    .description =
        "Enslave enemy planet population using an Orbiting Assault Platform",
    .handler = &enslave,
};

}  // namespace GB::commands
