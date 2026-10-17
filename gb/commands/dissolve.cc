// SPDX-License-Identifier: Apache-2.0

/// \file dissolve.cc
/// \brief Dissolve empire, destroying all ships and sectors.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;

module commands;

namespace {

/**
 * @brief Destroy all living ships owned by the dissolving player.
 */
std::vector<shipnum_t> destroy_empire_ships(GameObj& g, player_t playernum) {
  std::vector<shipnum_t> destroyed;
  for (auto ship_handle :
       ShipList(g.entity_manager, ShipList::IterationType::AllAlive)) {
    Ship& ship = *ship_handle;
    if (ship.owner() != playernum) {
      continue;
    }
    destroyed.push_back(ship.number());
    g.entity_manager.kill_ship(playernum, ship);
  }
  return destroyed;
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

    for (const auto& planet : PlanetList::readonly(
             g.entity_manager, star.star_id(), star.numplanets())) {
      g.entity_manager.mutate_planet_and_sectors(
          star.star_id(), planet.planet_order(),
          [&](Planet& pl, SectorMap& smap) {
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

  if (!g.is_leader()) {
    g.out << "Only the leader may dissolve the race. The "
             "leader has been notified of your "
             "attempt!!!\n";
    warn_player(g.session_registry, g.entity_manager, playernum,
                Race::leader_id,
                GB::presentation::render_dissolve_leader_warning(governor));
    return false;
  }

  if (argv.size() < 3) {
    g.out << "Self-Destruct sequence requires passwords.\n";
    g.out << "Please use 'dissolve <race password> <leader "
             "password>'<option> to initiate\n";
    g.out << "self-destruct sequence.\n";
    return false;
  }

  const bool waste = (argv.size() > 3 && argv[3].starts_with('w'));
  const auto [auth_player, auth_gov] =
      getracenum(g.entity_manager, argv[1], argv[2]);
  if (auth_player != playernum || !Race::is_leader(auth_gov)) {
    g.out << "Password mismatch, self-destruct not initiated!\n";
    return false;
  }

  GB::presentation::DissolveResult result{
      .player = playernum,
      .waste = waste,
      .destroyed_ships = destroy_empire_ships(g, playernum),
  };
  dissolve_empire_colonies(g, playernum, waste);

  g.entity_manager.mutate_race(playernum, [&](Race& race) {
    race.dissolved = true;
    result.race_name = race.name;
    post(g.entity_manager,
         GB::presentation::render_dissolve_declaration_news(result),
         NewsType::DECLARATION);
  });

  g.present(result);
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
