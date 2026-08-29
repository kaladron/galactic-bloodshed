// SPDX-License-Identifier: Apache-2.0

/// \file prompt.cc
/// \brief Generates player command prompts based on scope and orbit level.

module;

import std;
import dallib;

module gblib;

/**
 * \brief Create a prompt that shows the current AP and location of the player
 * \param g Game Object with player information
 * \return Prompt string for display to the user
 */
std::string do_prompt(const GameObj& g) {
  player_t Playernum = g.player();
  std::stringstream prompt;

  const auto* universe = g.entity_manager.peek_universe();
  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / )\n", universe->AP[Playernum]);
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(g.snum());
      prompt << std::format(" ( [{0}] /{1} )\n", star->AP(Playernum),
                            star->get_name());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(g.snum());
      prompt << std::format(" ( [{0}] /{1}/{2} )\n", star->AP(Playernum),
                            star->get_name(), star->get_planet_name(g.pnum()));
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.
  }

  const Ship* s = nullptr;
  try {
    s = g.entity_manager.peek_ship(g.shipno());
  } catch (const EntityNotFoundError&) {
    return " ( [?] /#? )\n";
  }

  switch (s->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] /#{1} )\n", universe->AP[Playernum],
                            g.shipno());
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      prompt << std::format(" ( [{0}] /{1}/#{2} )\n", star->AP(Playernum),
                            star->get_name(), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      prompt << std::format(" ( [{0}] /{1}/{2}/#{3} )\n", star->AP(Playernum),
                            star->get_name(), star->get_planet_name(g.pnum()),
                            g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.  (Ship within a ship)
  }

  /* I put this mess in because of non-functioning prompts when you
     are in a ship within a ship, or deeper. I am certain this can be
     done more elegantly (a lot more) but I don't feel like trying
     that right now. right now I want it to function. Maarten */
  const Ship* s2 = nullptr;
  try {
    s2 = g.entity_manager.peek_ship(s->destshipno());
  } catch (const EntityNotFoundError&) {
    return " ( [?] /#?/#? )\n";
  }

  switch (s2->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] /#{1}/#{2} )\n", universe->AP[Playernum],
                            s->destshipno(), g.shipno());
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      prompt << std::format(" ( [{0}] /{1}/#{2}/#{3} )\n", star->AP(Playernum),
                            star->get_name(), s->destshipno(), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      prompt << std::format(" ( [{0}] /{1}/{2}/#{3}/#{4} )\n",
                            star->AP(Playernum), star->get_name(),
                            star->get_planet_name(g.pnum()), s->destshipno(),
                            g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.  (Ship w/in ship w/in ship)
  }

  while (s2->whatorbits() == ScopeLevel::LEVEL_SHIP) {
    try {
      s2 = g.entity_manager.peek_ship(s2->destshipno());
    } catch (const EntityNotFoundError&) {
      return " ( [?] / /../#?/#? )\n";
    }
  }

  switch (s2->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / /../#{1}/#{2} )\n",
                            universe->AP[Playernum], s->destshipno(),
                            g.shipno());
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      prompt << std::format(" ( [{0}] /{1}/ /../#{2}/#{3} )\n",
                            star->AP(Playernum), star->get_name(),
                            s->destshipno(), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      prompt << std::format(" ( [{0}] /{1}/{2}/ /../#{3}/#{4} )\n",
                            star->AP(Playernum), star->get_name(),
                            star->get_planet_name(g.pnum()), s->destshipno(),
                            g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // (Ship w/in ship w/in ship w/in ship)
  }
  // Kidding!  All done. =)
  return prompt.str();
}
