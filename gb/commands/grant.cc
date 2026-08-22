// SPDX-License-Identifier: Apache-2.0

/// \file grant.cc
/// \brief Grant stars, ships, or treasury funds to governors.

module;

import gblib;
import notification;
import session;
import std;
import scnlib;

module commands;

namespace GB::commands {

bool grant(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  auto parsed_gov = scn::scan<int>(argv[1], "{}");
  if (!parsed_gov || parsed_gov->value() < 0 ||
      parsed_gov->value() > MAXGOVERNORS) {
    g.out << "Bad governor number.\n";
    return false;
  }
  governor_t gov{static_cast<unsigned char>(parsed_gov->value())};

  if (!g.race->governor[gov.value].active) {
    g.out << "That governor is not active.\n";
    return false;
  }

  auto race_handle = g.entity_manager.get_race(Playernum);
  auto& race = *race_handle;

  if (argv[2] == "star") {
    if (g.level() != ScopeLevel::LEVEL_STAR) {
      g.out << "Please cs to the star system first.\n";
      return false;
    }
    starnum_t snum = g.snum();
    auto star_handle = g.entity_manager.get_star(snum);
    if (!star_handle.get()) {
      g.out << "Star not found.\n";
      return false;
    }
    star_handle->governor(Playernum) = gov;
    warn_player(
        g.session_registry, g.entity_manager, Playernum, gov,
        std::format("\"{}\" has granted you control of the /{} star system.\n",
                    race.governor[Governor.value].name,
                    star_handle->get_name()));
    return true;
  }

  if (argv[2] == "ship") {
    if (argv.size() < 4) {
      g.out << "Syntax: grant <governor> ship <shiplist>\n";
      return false;
    }
    ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
    for (auto ship_handle : ships) {
      Ship& ship = *ship_handle;

      if (!ship_matches_filter(argv[3], ship)) continue;
      if (!authorized(Governor, ship)) continue;

      ship.governor() = gov;
      warn_player(g.session_registry, g.entity_manager, Playernum, gov,
                  std::format("\"{}\" granted you {} at {}\n",
                              race.governor[Governor.value].name, ship,
                              prin_ship_orbits(g.entity_manager, ship)));
      g.out << std::format("{} granted to \"{}\"\n", ship,
                           race.governor[gov.value].name);
    }
    return true;
  }

  if (argv[2] == "money") {
    if (argv.size() < 4) {
      g.out << "Indicate the amount of money.\n";
      return false;
    }
    auto parsed_amount = scn::scan<long>(argv[3], "{}");
    if (!parsed_amount) {
      g.out << "Invalid amount.\n";
      return false;
    }
    long amount = parsed_amount->value();
    if (amount < 0 && Governor != 0) {
      g.out << "Only leaders may make take away money.\n";
      return false;
    }
    if (amount > race.governor[Governor.value].money)
      amount = race.governor[Governor.value].money;
    else if (-amount > race.governor[gov.value].money)
      amount = -race.governor[gov.value].money;
    if (amount >= 0)
      g.out << std::format("{} money granted to \"{}\".\n", amount,
                           race.governor[gov.value].name);
    else
      g.out << std::format("{} money deducted from \"{}\".\n", -amount,
                           race.governor[gov.value].name);
    if (amount >= 0)
      warn_player(g.session_registry, g.entity_manager, Playernum, gov,
                  std::format("\"{}\" granted you {} money.\n",
                              race.governor[Governor.value].name, amount));
    else
      warn_player(g.session_registry, g.entity_manager, Playernum, gov,
                  std::format("\"{}\" docked you {} money.\n",
                              race.governor[Governor.value].name, -amount));
    race.governor[Governor.value].money -= amount;
    race.governor[gov.value].money += amount;
    return true;
  }

  g.out << "You can't grant that.\n";
  return false;
}

const CommandDescriptor grant_cmd{
    .name = "grant",
    .roles = {.leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "grant <governor #> <star|ship|money> [<shiplist|amount>]",
    .description =
        "Grant control of stars, ships, or treasury funds to a governor",
    .handler = &grant,
};

}  // namespace GB::commands
