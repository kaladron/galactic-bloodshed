// SPDX-License-Identifier: Apache-2.0

/// \file prompt.cc
/// \brief Generates player command prompts based on scope and orbit level.

module;

import std;
import dallib;

module gblib;

std::string format_ship_prompt(EntityManager& em, const player_t player,
                               const shipnum_t shipno) {
  const Ship* current_ship = em.peek_ship(shipno);

  std::vector<shipnum_t> ship_chain{shipno};
  std::unordered_set<shipnum_t> visited{shipno};

  while (current_ship->whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const shipnum_t parent_no = current_ship->destshipno();
    if (parent_no == 0 || visited.contains(parent_no)) {
      break;
    }
    visited.insert(parent_no);
    current_ship = em.peek_ship(parent_no);
    ship_chain.push_back(parent_no);
  }

  ap_t ap = 0;
  std::string path;

  switch (current_ship->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV: {
      const auto* universe = em.peek_universe();
      ap = universe->AP[player];
      break;
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = em.peek_star(current_ship->storbits());
      ap = star->AP(player);
      path = std::format("/{}", star->get_name());
      break;
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = em.peek_star(current_ship->storbits());
      ap = star->AP(player);
      path = std::format("/{}/{}", star->get_name(),
                         star->get_planet_name(current_ship->pnumorbits()));
      break;
    }
    case ScopeLevel::LEVEL_SHIP:
      std::unreachable();
  }

  for (const shipnum_t num : std::views::reverse(ship_chain)) {
    path += std::format("/#{}", num);
  }

  return std::format(" ( [{}] {} )\n", ap, path);
}

/**
 * \brief Create a prompt that shows the current AP and location of the player
 * \param g Game Object with player information
 * \return Prompt string for display to the user
 */
std::string do_prompt(const GameObj& g) {
  const player_t player = g.player();
  const auto* universe = g.entity_manager.peek_universe();

  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      return std::format(" ( [{}] / )\n", universe->AP[player]);

    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(g.snum());
      return std::format(" ( [{}] /{} )\n", star->AP(player), star->get_name());
    }

    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(g.snum());
      return std::format(" ( [{}] /{}/{} )\n", star->AP(player),
                         star->get_name(), star->get_planet_name(g.pnum()));
    }

    case ScopeLevel::LEVEL_SHIP:
      return format_ship_prompt(g.entity_manager, player, g.shipno());
  }
}
