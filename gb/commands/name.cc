// SPDX-License-Identifier: Apache-2.0

/// \file name.cc
/// \brief Name or rename game entities.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool name(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (!std::isalnum(argv[2][0])) {
    g.out << "Illegal name format.\n";
    return false;
  }

  std::string formatted_name = argv[2];
  for (std::size_t i = 3; i < argv.size(); i++) {
    formatted_name += " ";
    formatted_name += argv[i];
  }

  /* make sure there are no ^'s or '/' in name,
    also make sure the name has at least 1 character in it */
  bool has_invalid_char = std::ranges::any_of(formatted_name, [](char ch) {
    return (!std::isalnum(ch) && ch != ' ' && ch != '.') || ch == '/';
  });
  auto spaces = std::ranges::count(formatted_name, ' ');

  if (spaces == static_cast<long>(formatted_name.size())) {
    g.out << "Illegal name.\n";
    return false;
  }

  if (formatted_name.empty() || has_invalid_char) {
    g.out << std::format("Illegal name {}.\n",
                         has_invalid_char ? "form" : "length");
    return false;
  }

  if (argv[1] == "ship") {
    if (g.level() == ScopeLevel::LEVEL_SHIP) {
      auto ship = g.entity_manager.get_ship(g.shipno());
      if (!ship.get()) {
        g.out << "Ship not found.\n";
        return false;
      }
      ship->name() = formatted_name;
      g.out << "Name set.\n";
      return true;
    }
    g.out << "You have to 'cs' to a ship to name it.\n";
    return false;
  }
  if (argv[1] == "class") {
    if (g.level() == ScopeLevel::LEVEL_SHIP) {
      auto ship = g.entity_manager.get_ship(g.shipno());
      if (!ship.get()) {
        g.out << "Ship not found.\n";
        return false;
      }
      if (ship->type() != ShipType::OTYPE_FACTORY) {
        g.out << "You are not at a factory!\n";
        return false;
      }
      if (ship->on()) {
        g.out << "This factory is already on line.\n";
        return false;
      }
      ship->shipclass() = formatted_name;
      g.out << "Class set.\n";
      return true;
    }
    g.out << "You have to 'cs' to a factory to name the ship class.\n";
    return false;
  }
  if (argv[1] == "block") {
    /* name your alliance block */
    if (Governor != 0) {
      g.out << "You are not authorized to do this.\n";
      return false;
    }
    try {
      auto block_handle = g.entity_manager.get_block(Playernum.value);
      auto& block = *block_handle;
      block.name = formatted_name;
    } catch (const EntityNotFoundError&) {
      g.out << "Block not found.\n";
      return false;
    }
    g.out << "Done.\n";
    return true;
  }
  if (argv[1] == "star") {
    if (g.level() == ScopeLevel::LEVEL_STAR) {
      if (!g.race->God) {
        g.out << "Only dieties may name a star.\n";
        return false;
      }
      auto star = g.entity_manager.get_star(g.snum());
      if (!star.get()) {
        g.out << "Star not found.\n";
        return false;
      }
      star->set_name(formatted_name);
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
      auto star = g.entity_manager.get_star(g.snum());
      if (!star.get()) {
        g.out << "Star not found.\n";
        return false;
      }
      star->set_planet_name(g.pnum(), formatted_name);
      return true;
    }
    g.out << "You have to 'cs' to a planet to name it.\n";
    return false;
  }
  if (argv[1] == "race") {
    if (Governor != 0) {
      g.out << "You are not authorized to do this.\n";
      return false;
    }
    auto race = g.entity_manager.get_race(Playernum);
    if (!race.get()) {
      g.out << "Race not found.\n";
      return false;
    }
    race->name = formatted_name;
    g.out << std::format("Name changed to `{}'.\n", race->name);
    return true;
  }
  if (argv[1] == "governor") {
    auto race = g.entity_manager.get_race(Playernum);
    if (!race.get()) {
      g.out << "Race not found.\n";
      return false;
    }
    race->governor[Governor.value].name = formatted_name;
    g.out << std::format("Name changed to `{}'.\n",
                         race->governor[Governor.value].name);
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