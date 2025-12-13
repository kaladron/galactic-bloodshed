// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import scnlib;
import std;

module commands;

namespace GB::commands {
void arm(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int mode;
  if (argv[0] == "arm") {
    mode = 1;
  } else {
    mode = 0;  // disarm
  }
  int max_allowed;
  int amount = 0;
  money_t cost = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Change scope to planet level first.\n";
    return;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum);
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet_handle = g.entity_manager.get_planet(g.snum, g.pnum);
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& planet = *planet_handle;

  if (planet.slaved_to() > 0 && planet.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return;
  }

  auto xy_result = scn::scan<int, int>(argv[1], "{},{}");
  if (!xy_result) {
    g.out << "Bad format for sector.\n";
    return;
  }
  auto [x, y] = xy_result->values();
  if (x < 0 || y < 0 || x > planet.Maxx() - 1 || y > planet.Maxy() - 1) {
    g.out << "Illegal coordinates.\n";
    return;
  }

  auto smap_handle = g.entity_manager.get_sectormap(g.snum, g.pnum);
  if (!smap_handle.get()) {
    g.out << "Sector map not found.\n";
    return;
  }
  auto& smap = *smap_handle;
  auto& sect = smap.get(x, y);
  if (sect.get_owner() != Playernum) {
    g.out << "You don't own that sector.\n";
    return;
  }
  if (mode) {
    max_allowed = MIN(sect.get_popn(), planet.info(Playernum - 1).destruct *
                                           (sect.get_mobilization() + 1));
    if (argv.size() < 3)
      amount = max_allowed;
    else {
      amount = std::stoul(argv[2]);
      if (amount <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return;
      }
    }
    amount = std::min(amount, max_allowed);
    if (!amount) {
      g.out << "You can't arm any civilians now.\n";
      return;
    }
    /*    enlist_cost = ENLIST_TROOP_COST * amount; */
    money_t enlist_cost = g.race->fighters * amount;
    if (enlist_cost > g.race->governor[Governor].money) {
      g.out << std::format("You need {} money to enlist {} troops.\n",
                           enlist_cost, amount);
      return;
    }
    auto race_handle = g.entity_manager.get_race(Playernum);
    auto& race_mut = *race_handle;
    race_mut.governor[Governor].money -= enlist_cost;

    cost = std::max(1U, amount / (sect.get_mobilization() + 1));
    sect.set_troops(sect.get_troops() + amount);
    sect.set_popn(sect.get_popn() - amount);
    planet.popn() -= amount;
    planet.info(Playernum - 1).popn -= amount;
    planet.troops() += amount;
    planet.info(Playernum - 1).troops += amount;
    planet.info(Playernum - 1).destruct -= cost;
    g.out << std::format(
        "{} population armed at a cost of {} (now {} civilians, {} military)\n",
        amount, cost, sect.get_popn(), sect.get_troops());
    g.out << std::format("This mobilization cost {} money.\n", enlist_cost);
  } else {
    if (argv.size() < 3)
      amount = sect.get_troops();
    else {
      amount = std::stoi(argv[2]);
      if (amount <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return;
      }
      amount = MIN(sect.get_troops(), amount);
    }
    sect.set_popn(sect.get_popn() + amount);
    sect.set_troops(sect.get_troops() - amount);
    planet.popn() += amount;
    planet.troops() -= amount;
    planet.info(Playernum - 1).popn += amount;
    planet.info(Playernum - 1).troops -= amount;
    g.out << std::format("{} troops disarmed (now {} civilians, {} military)\n",
                         amount, sect.get_popn(), sect.get_troops());
  }
}
}  // namespace GB::commands