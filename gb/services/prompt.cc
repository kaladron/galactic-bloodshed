// SPDX-License-Identifier: Apache-2.0

/// \file prompt.cc
/// \brief Generates player command prompts based on scope and orbit level.

module;

import std;
import dallib;
import gb.repositories.glaze;
import glaze.core;
import glaze.json;

module gb.services;

struct PromptJsonEnvelope {
  std::string_view type{"prompt"};
  const PromptViewModel* data{nullptr};
};

PromptViewModel build_ship_prompt(EntityManager& em, const player_t player,
                                  const shipnum_t shipno) {
  const Ship* current_ship = em.peek_ship(shipno);

  std::vector<shipnum_t> ship_chain{shipno};

  while (current_ship->whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const shipnum_t parent_no = current_ship->destshipno().value();
    current_ship = em.peek_ship(parent_no);
    ship_chain.push_back(parent_no);
  }

  ap_t ap = 0;
  std::string star_name;
  std::string planet_name;
  std::string path;

  switch (current_ship->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV: {
      const auto* universe = em.peek_universe();
      ap = universe->get_AP(player);
      break;
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = em.peek_star(current_ship->storbits());
      ap = star->AP(player);
      star_name = star->get_name();
      path = std::format("/{}", star_name);
      break;
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = em.peek_star(current_ship->storbits());
      ap = star->AP(player);
      star_name = star->get_name();
      planet_name = star->get_planet_name(current_ship->pnumorbits());
      path = std::format("/{}/{}", star_name, planet_name);
      break;
    }
    case ScopeLevel::LEVEL_SHIP:
      std::unreachable();
  }

  for (const shipnum_t num : std::views::reverse(ship_chain)) {
    path += std::format("/#{}", num);
  }

  return PromptViewModel{
      .ap = ap,
      .level = ScopeLevel::LEVEL_SHIP,
      .star = std::move(star_name),
      .planet = std::move(planet_name),
      .ship = std::format("#{}", shipno),
      .path = std::move(path),
  };
}

PromptViewModel build_prompt(const GameObj& g) {
  const player_t player = g.player();
  const auto* universe = g.entity_manager.peek_universe();

  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      return PromptViewModel{
          .ap = universe->get_AP(player),
          .level = ScopeLevel::LEVEL_UNIV,
          .path = "/",
      };

    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(g.snum());
      std::string star_name = star->get_name();
      std::string path = std::format("/{}", star_name);
      return PromptViewModel{
          .ap = star->AP(player),
          .level = ScopeLevel::LEVEL_STAR,
          .star = std::move(star_name),
          .path = std::move(path),
      };
    }

    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(g.snum());
      std::string star_name = star->get_name();
      std::string planet_name = star->get_planet_name(g.pnum());
      std::string path = std::format("/{}/{}", star_name, planet_name);
      return PromptViewModel{
          .ap = star->AP(player),
          .level = ScopeLevel::LEVEL_PLAN,
          .star = std::move(star_name),
          .planet = std::move(planet_name),
          .path = std::move(path),
      };
    }

    case ScopeLevel::LEVEL_SHIP:
      return build_ship_prompt(g.entity_manager, player, g.shipno());
  }
}

std::string format_ship_prompt(EntityManager& em, const player_t player,
                               const shipnum_t shipno) {
  const auto vm = build_ship_prompt(em, player, shipno);
  return std::format(" ( [{}] {} )\n", vm.ap, vm.path);
}

/**
 * \brief Create a prompt that shows the current AP and location of the player
 * \param g Game Object with player information
 * \return Prompt string for display to the user
 */
std::string do_prompt(const GameObj& g) {
  const auto vm = build_prompt(g);
  if (g.ui_mode() == GB::presentation::UiMode::JSON) {
    std::string out;
    const PromptJsonEnvelope envelope{.data = &vm};
    if (const auto ec = glz::write_json(envelope, out); !ec) {
      out.push_back('\n');
    }
    return out;
  }
  return std::format(" ( [{}] {} )\n", vm.ap, vm.path);
}
