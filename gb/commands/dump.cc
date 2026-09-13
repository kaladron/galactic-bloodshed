// SPDX-License-Identifier: Apache-2.0

/// \file dump.cc
/// \brief Transfer exploration data to another player.

module;

import gb.entities;
import gb.services;
import std;
import notification;
import session;

module commands;

namespace GB::commands {

static void transfer_star_data(EntityManager& em, player_t donor_id,
                               player_t recipient_id, Star& star) {
  if (!star.is_explored_by(donor_id)) {
    return;
  }
  star.mark_explored_by(recipient_id);
  const starnum_t star_id = star.get_struct().star_id;
  for (auto planet_handle : PlanetList(em, star_id, star)) {
    auto& planet = *planet_handle;
    if (planet.info(donor_id).explored) {
      planet.info(recipient_id).explored = 1;
    }
  }
}

bool dump(const command_t& argv, GameObj& g) {
  const player_t donor_id = g.player();

  const player_t recipient_id = get_player(g.entity_manager, argv[1]);
  if (recipient_id.value == 0) {
    g.out << "No such player.\n";
    return false;
  }

  // Transfer all planet and star knowledge to the recipient
  if (argv.size() < 3) {
    for (auto current_star_handle : StarList(g.entity_manager)) {
      transfer_star_data(g.entity_manager, donor_id, recipient_id,
                         *current_star_handle);
    }
  } else { /* list of places given */
    for (const auto& place_arg : argv | std::views::drop(2)) {
      Place where{g, place_arg, true};
      if (!where.err && where.level != ScopeLevel::LEVEL_UNIV &&
          where.level != ScopeLevel::LEVEL_SHIP) {
        g.entity_manager.mutate_star(where.snum, [&](Star& current_star) {
          transfer_star_data(g.entity_manager, donor_id, recipient_id,
                             current_star);
        });
      }
    }
  }

  warn_race(g.session_registry, g.entity_manager, recipient_id,
            std::format("{} [{}] has given you exploration data.\n",
                        g.race->name, donor_id));
  g.out << "Exploration Data transferred.\n";
  return true;
}

const CommandDescriptor dump_cmd{
    .name = "dump",
    .roles =
        {
            .no_guests = true,
            .leader_only = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::fixed_star(10),
    .min_args = 2,
    .syntax = "dump <player> [<place> ...]",
    .description =
        "Transfer exploration data about stars/planets to another player",
    .handler = &dump,
};

}  // namespace GB::commands
