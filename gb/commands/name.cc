// SPDX-License-Identifier: Apache-2.0

/// \file name.cc
/// \brief Name or rename game entities.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool name(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (argv[2].empty() ||
      !std::isalnum(static_cast<unsigned char>(argv[2][0]))) {
    g.out << "Illegal name format.\n";
    return false;
  }

  std::string formatted_name = argv[2];
  for (std::size_t i = 3; i < argv.size(); i++) {
    formatted_name += " ";
    formatted_name += argv[i];
  }

  const bool has_invalid_char =
      std::ranges::any_of(formatted_name, [](char ch) {
        return !std::isalnum(static_cast<unsigned char>(ch)) && ch != ' ' &&
               ch != '.';
      });
  if (has_invalid_char) {
    g.out << "Illegal name form.\n";
    return false;
  }

  if (argv[1] == "ship") {
    if (g.level() == ScopeLevel::LEVEL_SHIP) {
      g.entity_manager.mutate_ship(
          g.shipno(), [&](Ship& ship) { ship.name() = formatted_name; });
      g.present(GB::presentation::NameResult{
          .target = GB::presentation::NameTarget::Ship,
          .name = formatted_name,
      });
      return true;
    }
    g.out << "You have to 'cs' to a ship to name it.\n";
    return false;
  }
  if (argv[1] == "class") {
    if (g.level() == ScopeLevel::LEVEL_SHIP) {
      bool ok = false;
      g.entity_manager.mutate_ship(g.shipno(), [&](Ship& ship) {
        if (ship.type() != ShipType::OTYPE_FACTORY) {
          g.out << "You are not at a factory!\n";
          return;
        }
        if (ship.on()) {
          g.out << "This factory is already on line.\n";
          return;
        }
        ship.shipclass() = formatted_name;
        ok = true;
      });
      if (ok) {
        g.present(GB::presentation::NameResult{
            .target = GB::presentation::NameTarget::Class,
            .name = formatted_name,
        });
      }
      return ok;
    }
    g.out << "You have to 'cs' to a factory to name the ship class.\n";
    return false;
  }
  if (argv[1] == "block") {
    /* name your alliance block */
    if (!g.is_leader()) {
      g.out << "You are not authorized to do this.\n";
      return false;
    }
    try {
      g.entity_manager.mutate_block(
          Playernum, [&](Block& b) { b.name() = formatted_name; });
    } catch (const EntityNotFoundError&) {
      g.out << "Block not found.\n";
      return false;
    }
    g.present(GB::presentation::NameResult{
        .target = GB::presentation::NameTarget::Block,
        .name = formatted_name,
    });
    return true;
  }
  if (argv[1] == "star") {
    if (g.level() == ScopeLevel::LEVEL_STAR) {
      if (!g.race->God) {
        g.out << "Only dieties may name a star.\n";
        return false;
      }
      g.entity_manager.mutate_star(
          g.snum(), [&](Star& star) { star.set_name(formatted_name); });
      g.present(GB::presentation::NameResult{
          .target = GB::presentation::NameTarget::Star,
          .name = formatted_name,
      });
      return true;
    }
    g.out << "You have to 'cs' to a star to name it.\n";
    return false;
  }
  if (argv[1] == "planet") {
    if (g.level() == ScopeLevel::LEVEL_PLAN) {
      if (!g.race->God) {
        g.out << "Only deity can rename planets.\n";
        return false;
      }
      g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
        star.set_planet_name(g.pnum(), formatted_name);
      });
      g.present(GB::presentation::NameResult{
          .target = GB::presentation::NameTarget::Planet,
          .name = formatted_name,
      });
      return true;
    }
    g.out << "You have to 'cs' to a planet to name it.\n";
    return false;
  }
  if (argv[1] == "race") {
    if (!g.is_leader()) {
      g.out << "You are not authorized to do this.\n";
      return false;
    }
    g.entity_manager.mutate_race(
        Playernum, [&](Race& race) { race.name = formatted_name; });
    g.present(GB::presentation::NameResult{
        .target = GB::presentation::NameTarget::Race,
        .name = formatted_name,
    });
    return true;
  }
  if (argv[1] == "governor") {
    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      race.governor(Governor).name = formatted_name;
    });
    g.present(GB::presentation::NameResult{
        .target = GB::presentation::NameTarget::Governor,
        .name = formatted_name,
    });
    return true;
  }

  g.out << "I don't know what you mean.\n";
  return false;
}

const CommandDescriptor name_cmd{
    .name = "name",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "name <ship|class|block|star|planet|race|governor> <name>",
    .description =
        "Name or rename game entities, ships, governors, and alliance blocks",
    .handler = &name,
};

}  // namespace GB::commands