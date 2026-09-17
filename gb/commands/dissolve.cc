// SPDX-License-Identifier: Apache-2.0

/// \file dissolve.cc
/// \brief Dissolve empire, destroying all ships and sectors.

module;

import std;
import gb.entities;
import gb.services;
import notification;
import session;

module commands;

namespace {

/**
 * @brief Destroy all living ships owned by the dissolving player.
 */
void destroy_empire_ships(GameObj& g, player_t playernum) {
  for (auto ship_handle :
       ShipList(g.entity_manager, ShipList::IterationType::AllAlive)) {
    Ship& ship = *ship_handle;
    if (ship.owner() != playernum) {
      continue;
    }
    g.entity_manager.kill_ship(playernum, ship);
    g.out << std::format("Ship #{}, self-destruct enabled\n", ship.number());
  }
}

/**
 * @brief Clear all planetary stockpiles, sector ownership, and star
 * inhabitation flags for the dissolving player.
 */
void dissolve_empire_colonies(GameObj& g, player_t playernum, bool waste) {
  for (auto star_handle : StarList(g.entity_manager)) {
    Star& star = *star_handle;
    if (!star.is_explored_by(playernum)) {
      continue;
    }

    for (planetnum_t pnum = 0; pnum < star.numplanets(); ++pnum) {
      g.entity_manager.mutate_planet_and_sectors(
          star.star_id(), pnum, [&](Planet& pl, SectorMap& smap) {
            auto& pinfo = pl.info(playernum);
            pinfo.fuel = 0;
            pinfo.destruct = 0;
            pinfo.resource = 0;
            pinfo.popn = 0;
            pinfo.troops = 0;
            pinfo.tax = 0;
            pinfo.newtax = 0;
            pinfo.crystals = 0;
            pinfo.numsectsowned = 0;
            pinfo.explored = 0;
            pinfo.autorep = 0;

            for (auto& s : smap) {
              if (s.get_owner() == playernum) {
                s.set_owner(0);
                s.set_troops(0);
                s.clear_popn();
                if (waste) {
                  s.set_condition(SectorType::SEC_WASTED);
                }
              }
            }
            pl.sync_demographics(smap);
          });
    }
    star.clear_inhabited_by(playernum);
  }
}

}  // namespace

namespace GB::commands {

bool dissolve(const command_t& argv, GameObj& g) {
  const player_t playernum = g.player();
  const governor_t governor = g.governor();
  if (!DISSOLVE) {
    g.out << "Dissolve has been disabled. Please notify diety.\n";
    return false;
  }

  if (governor != 0) {
    g.out << "Only the leader may dissolve the race. The "
             "leader has been notified of your "
             "attempt!!!\n";
    g.session_registry.notify_player(
        playernum, 0,
        std::format("Governor #{} has attempted to dissolve this race.\n",
                    governor));
    return false;
  }

  if (argv.size() < 3) {
    g.out << "Self-Destruct sequence requires passwords.\n";
    g.out << "Please use 'dissolve <race password> <leader "
             "password>'<option> to initiate\n";
    g.out << "self-destruct sequence.\n";
    return false;
  }
  g.out << "WARNING!! WARNING!! WARNING!!\n";
  g.out << "-------------------------------\n";
  g.out << "Entering self destruct sequence!\n";

  const bool waste = (argv.size() > 3 && argv[3].starts_with('w'));
  const auto [auth_player, auth_gov] =
      getracenum(g.entity_manager, argv[1], argv[2]);
  if (auth_player != playernum || auth_gov != 0) {
    g.out << "Password mismatch, self-destruct not initiated!\n";
    return false;
  }

  destroy_empire_ships(g, playernum);
  dissolve_empire_colonies(g, playernum, waste);

  g.entity_manager.mutate_race(playernum, [&](Race& race) {
    race.dissolved = true;
    post(g.entity_manager,
         std::format("{} [{}] has dissolved.\n", race.name, playernum),
         NewsType::DECLARATION);
  });

  return true;
}

const CommandDescriptor dissolve_cmd{
    .name = "dissolve",
    .roles =
        {
            .no_guests = true,
            .leader_only = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "dissolve <race password> <leader password> [waste]",
    .description = "Dissolve empire, destroying all ships and sectors",
    .handler = &dissolve,
};

}  // namespace GB::commands
