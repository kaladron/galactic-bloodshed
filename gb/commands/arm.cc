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
  const bool is_arm = (argv[0] == "arm");

  const auto& planet_peek = *g.entity_manager.peek_planet(g.snum(), g.pnum());
  if (planet_peek.slaved_to() > 0 && planet_peek.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return false;
  }

  auto coords_opt = Coordinates::parse(argv[1]);
  if (!coords_opt) {
    g.out << "Bad format for sector.\n";
    return false;
  }
  const Coordinates coords = *coords_opt;
  if (!planet_peek.is_valid(coords)) {
    g.out << "Illegal coordinates.\n";
    return false;
  }

  const auto& smap_peek = *g.entity_manager.peek_sectormap(g.snum(), g.pnum());
  const auto& sect_peek = smap_peek.get(coords);
  if (sect_peek.get_owner() != Playernum) {
    g.out << "You don't own that sector.\n";
    return false;
  }

  if (is_arm) {
    population_t max_allowed = std::min(
        sect_peek.get_popn(),
        static_cast<population_t>(planet_peek.info(Playernum).destruct *
                                  (sect_peek.get_mobilization() + 1)));
    population_t amount = 0;
    if (argv.size() < 3) {
      amount = max_allowed;
    } else {
      auto count_res = scn::scan<population_t>(argv[2], "{}");
      if (!count_res || count_res->value() <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return false;
      }
      amount = count_res->value();
    }
    amount = std::min(amount, max_allowed);
    if (!amount) {
      g.out << "You can't arm any civilians now.\n";
      return false;
    }

    money_t enlist_cost = g.race->fighters * amount;
    if (enlist_cost > g.race->governor[Governor.value].money) {
      g.out << std::format("You need {} money to enlist {} troops.\n",
                           enlist_cost, amount);
      return false;
    }

    g.entity_manager.mutate_race(Playernum, [&](Race& race_mut) {
      race_mut.governor[Governor.value].money -= enlist_cost;
    });

    money_t cost = std::max(
        1U,
        static_cast<unsigned int>(amount / (sect_peek.get_mobilization() + 1)));
    population_t final_popn = 0;
    population_t final_troops = 0;

    g.entity_manager.mutate_sectormap(g.snum(), g.pnum(), [&](SectorMap& smap) {
      auto& sect = smap.get(coords);
      sect.set_troops(sect.get_troops() + amount);
      sect.subtract_popn(amount);
      final_popn = sect.get_popn();
      final_troops = sect.get_troops();
    });

    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.popn() -= amount;
      planet.info(Playernum).popn -= amount;
      planet.troops() += amount;
      planet.info(Playernum).troops += amount;
      planet.info(Playernum).destruct -= cost;
    });

    g.out << std::format(
        "{} population armed at a cost of {} (now {} civilians, {} military)\n",
        amount, cost, final_popn, final_troops);
    g.out << std::format("This mobilization cost {} money.\n", enlist_cost);
  } else {
    population_t amount = 0;
    if (argv.size() < 3) {
      amount = sect_peek.get_troops();
    } else {
      auto count_res = scn::scan<population_t>(argv[2], "{}");
      if (!count_res || count_res->value() <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return false;
      }
      amount = std::min(sect_peek.get_troops(), count_res->value());
    }

    population_t final_popn = 0;
    population_t final_troops = 0;

    g.entity_manager.mutate_sectormap(g.snum(), g.pnum(), [&](SectorMap& smap) {
      auto& sect = smap.get(coords);
      sect.add_popn(amount);
      sect.set_troops(sect.get_troops() - amount);
      final_popn = sect.get_popn();
      final_troops = sect.get_troops();
    });

    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.popn() += amount;
      planet.troops() -= amount;
      planet.info(Playernum).popn += amount;
      planet.info(Playernum).troops -= amount;
    });

    g.out << std::format("{} troops disarmed (now {} civilians, {} military)\n",
                         amount, final_popn, final_troops);
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