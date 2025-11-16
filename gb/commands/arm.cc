// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void arm(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int mode;
  if (argv[0] == "arm") {
    mode = 1;
  } else {
    mode = 0;  // disarm
  }
  int x = -1;
  int y = -1;
  int max_allowed;
  int amount = 0;
  money_t cost = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Change scope to planet level first.\n";
    return;
  }
  if (!stars[g.snum].control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet = getplanet(g.snum, g.pnum);

  if (planet.slaved_to() > 0 && planet.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return;
  }

  std::sscanf(argv[1].c_str(), "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet.Maxx() - 1 || y > planet.Maxy() - 1) {
    g.out << "Illegal coordinates.\n";
    return;
  }

  auto sect = getsector(planet, x, y);
  if (sect.owner != Playernum) {
    g.out << "You don't own that sector.\n";
    return;
  }
  if (mode) {
    max_allowed = MIN(sect.popn, planet.info(Playernum - 1).destruct *
                                     (sect.mobilization + 1));
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
    auto &race = races[Playernum - 1];
    /*    enlist_cost = ENLIST_TROOP_COST * amount; */
    money_t enlist_cost = race.fighters * amount;
    if (enlist_cost > race.governor[Governor].money) {
      g.out << std::format("You need {} money to enlist {} troops.\n",
                           enlist_cost, amount);
      return;
    }
    race.governor[Governor].money -= enlist_cost;
    putrace(race);

    cost = std::max(1U, amount / (sect.mobilization + 1));
    sect.troops += amount;
    sect.popn -= amount;
    planet.popn() -= amount;
    planet.info(Playernum - 1).popn -= amount;
    planet.troops() += amount;
    planet.info(Playernum - 1).troops += amount;
    planet.info(Playernum - 1).destruct -= cost;
    g.out << std::format(
        "{} population armed at a cost of {} (now {} civilians, {} military)\n",
        amount, cost, sect.popn, sect.troops);
    g.out << std::format("This mobilization cost {} money.\n", enlist_cost);
  } else {
    if (argv.size() < 3)
      amount = sect.troops;
    else {
      amount = std::stoi(argv[2]);
      if (amount <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return;
      }
      amount = MIN(sect.troops, amount);
    }
    sect.popn += amount;
    sect.troops -= amount;
    planet.popn() += amount;
    planet.troops() -= amount;
    planet.info(Playernum - 1).popn += amount;
    planet.info(Playernum - 1).troops -= amount;
    g.out << std::format("{} troops disarmed (now {} civilians, {} military)\n",
                         amount, sect.popn, sect.troops);
  }
  putsector(sect, planet, x, y);
  putplanet(planet, stars[g.snum], g.pnum);
}
}  // namespace GB::commands