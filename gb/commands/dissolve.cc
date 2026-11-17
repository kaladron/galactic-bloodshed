// SPDX-License-Identifier: Apache-2.0

/// \file dissolve.cc
/// \brief Dissolve empire, destroying all ships and sectors.

module;

import std;
import gb.entities;
import gb.mechanics;
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
    kill_ship(g.entity_manager, playernum, ship);
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
    for (const auto& planet :
         PlanetList::readonly(g.entity_manager, star.star_id(), star)) {
      if (planet.slaved_to() == playernum) {
        g.entity_manager.mutate_planet(
            star.star_id(), planet.planet_order(),
            [](Planet& pl) { pl.slaved_to() = std::nullopt; });
      }
      if (star.is_explored_by(playernum)) {
        g.entity_manager.mutate_planet_and_sectors(
            star.star_id(), planet.planet_order(),
            [&](Planet& pl, SectorMap& smap) {
              pl.info_map().erase(playernum);
              for (auto& s : smap.owned_by(playernum)) {
                if (waste) {
                  s.devastate();
                } else {
                  s.clear_colony();
                }
              }
              pl.sync_demographics(smap);
            });
      }
    }
    star.clear_inhabited_by(playernum);
  }
}

void scrub_empire_market_and_blocks(EntityManager& em, player_t playernum) {
  {
    auto barrier = em.create_deletion_barrier();
    for (auto commod_handle : CommodList(em)) {
      auto& commod = *commod_handle;
      if (commod.owner() == playernum) {
        em.delete_commod(commod.id());
      } else if (commod.bidder() == playernum) {
        commod.clear_bid();
      }
    }
  }
  em.mutate_power(playernum, [&](Power& p) { p = Power{{.id = playernum}}; });
  for (auto block_handle : BlockList(em)) {
    auto& block = *block_handle;
    if (block.Playernum() == playernum) {
      block.invited().clear();
      block.pledged().clear();
      block.clear_power_stats();
      block.systems_owned() = 0;
      block.VPs() = 0;
    } else {
      block.uninvite(playernum);
      block.unpledge(playernum);
    }
  }
  compute_power_blocks(em);
}

}  // namespace

namespace GB::commands {

bool dissolve(const command_t& argv, GameObj& g, bool dissolve_enabled) {
  const player_t playernum = g.player();
  const governor_t governor = g.governor();
  if (!dissolve_enabled) {
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
  scrub_empire_market_and_blocks(g.entity_manager, playernum);
  g.session_registry.disconnect_race(playernum,
                                     "Your empire has been dissolved.\n");

  g.present(result);
  return true;
}

bool dissolve(const command_t& argv, GameObj& g) {
  return dissolve(argv, g, DISSOLVE);
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
