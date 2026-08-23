// SPDX-License-Identifier: Apache-2.0

/// \file arm.cc
/// \brief Arm or disarm sector populations.

module;

import gblib;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool arm(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  int mode;
  if (argv[0] == "arm") {
    mode = 1;
  } else {
    mode = 0;  // disarm
  }
  int max_allowed;
  int amount = 0;
  money_t cost = 0;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "Change scope to planet level first.\n";
    return false;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return false;
  }
  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return false;
  }
  auto& planet = *planet_handle;

  if (planet.slaved_to() > 0 && planet.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return false;
  }

  auto coords_opt = Coordinates::parse(argv[1]);
  if (!coords_opt) {
    g.out << "Bad format for sector.\n";
    return false;
  }
  const Coordinates coords = *coords_opt;
  if (!planet.is_valid(coords)) {
    g.out << "Illegal coordinates.\n";
    return false;
  }

  auto smap_handle = g.entity_manager.get_sectormap(g.snum(), g.pnum());
  if (!smap_handle.get()) {
    g.out << "Sector map not found.\n";
    return false;
  }
  auto& smap = *smap_handle;
  auto& sect = smap.get(coords);
  if (sect.get_owner() != Playernum) {
    g.out << "You don't own that sector.\n";
    return false;
  }
  if (mode) {
    max_allowed = MIN(sect.get_popn(), planet.info(Playernum).destruct *
                                           (sect.get_mobilization() + 1));
    if (argv.size() < 3)
      amount = max_allowed;
    else {
      try {
        amount = std::stoi(argv[2]);
        if (amount <= 0) {
          g.out << "You must specify a positive number of civs to arm.\n";
          return false;
        }
      } catch (const std::exception&) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return false;
      }
    }
    amount = std::min(amount, max_allowed);
    if (!amount) {
      g.out << "You can't arm any civilians now.\n";
      return false;
    }
    /*    enlist_cost = ENLIST_TROOP_COST * amount; */
    money_t enlist_cost = g.race->fighters * amount;
    if (enlist_cost > g.race->governor[Governor.value].money) {
      g.out << std::format("You need {} money to enlist {} troops.\n",
                           enlist_cost, amount);
      return false;
    }
    auto race_handle = g.entity_manager.get_race(Playernum);
    auto& race_mut = *race_handle;
    race_mut.governor[Governor.value].money -= enlist_cost;

    cost = std::max(1U, amount / (sect.get_mobilization() + 1));
    sect.set_troops(sect.get_troops() + amount);
    sect.subtract_popn(amount);
    planet.popn() -= amount;
    planet.info(Playernum).popn -= amount;
    planet.troops() += amount;
    planet.info(Playernum).troops += amount;
    planet.info(Playernum).destruct -= cost;
    g.out << std::format(
        "{} population armed at a cost of {} (now {} civilians, {} military)\n",
        amount, cost, sect.get_popn(), sect.get_troops());
    g.out << std::format("This mobilization cost {} money.\n", enlist_cost);
  } else {
    if (argv.size() < 3)
      amount = sect.get_troops();
    else {
      try {
        amount = std::stoi(argv[2]);
        if (amount <= 0) {
          g.out << "You must specify a positive number of civs to arm.\n";
          return false;
        }
      } catch (const std::exception&) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return false;
      }
      amount = MIN(sect.get_troops(), amount);
    }
    sect.add_popn(amount);
    sect.set_troops(sect.get_troops() - amount);
    planet.popn() += amount;
    planet.troops() -= amount;
    planet.info(Playernum).popn += amount;
    planet.info(Playernum).troops -= amount;
    g.out << std::format("{} troops disarmed (now {} civilians, {} military)\n",
                         amount, sect.get_popn(), sect.get_troops());
  }
  return true;
}

const CommandDescriptor arm_cmd{
    .name = "arm",
    .roles = {.no_guests = true, .star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "arm <sector x,y> [<# of civs>]",
    .description = "Convert civilian population to military units",
    .handler = &arm,
};

const CommandDescriptor disarm_cmd{
    .name = "disarm",
    .roles = {.no_guests = true, .star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "disarm <sector x,y> [<# of troops>]",
    .description = "Convert military units back to civilian population",
    .handler = &arm,
};

}  // namespace GB::commands