// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void insurgency(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 10;
  int who;
  int eligible;
  int them = 0;
  double x;
  int changed_hands;
  int chance;
  int i;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must 'cs' to the planet you wish to try it on.\n";
    return;
  }
  if (!stars[g.snum].control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  /*  if(argv.size()<3) {
        notify(Playernum, Governor, "The correct syntax is 'insurgency <race>
    <money>'\n");
        return;
    }*/
  if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1), APcount))
    return;
  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  auto& race = races[Playernum - 1];
  auto& alien = races[who - 1];
  if (alien.Guest) {
    g.out << "Don't be such a dickweed.\n";
    return;
  }
  if (who == Playernum) {
    g.out << "You can't revolt against yourself!\n";
    return;
  }
  eligible = 0;
  them = 0;
  for (i = 0; i < stars[g.snum].numplanets(); i++) {
    auto p = getplanet(g.snum, i);
    eligible += p.info(Playernum - 1).popn;
    them += p.info(who - 1).popn;
  }
  if (!eligible) {
    g.out << "You must have population in the star system to attempt "
             "insurgency\n.";
    return;
  }
  auto p = getplanet(g.snum, g.pnum);

  if (!p.info(who - 1).popn) {
    g.out << "This player does not occupy this planet.\n";
    return;
  }

  int amount = std::stoi(argv[2]);
  if (amount < 0) {
    g.out << "You have to use a positive amount of money.\n";
    return;
  }
  if (race.governor[Governor].money < amount) {
    g.out << "Nice try.\n";
    return;
  }

  x = INSURG_FACTOR * (double)amount * (double)p.info(who - 1).tax /
      (double)p.info(who - 1).popn;
  x *= morale_factor((double)(race.morale - alien.morale));
  x *= morale_factor((double)(eligible - them) / 50.0);
  x *= morale_factor(10.0 *
                     (double)(race.fighters * p.info(Playernum - 1).troops -
                              alien.fighters * p.info(who - 1).troops)) /
       50.0;
  notify(Playernum, Governor, std::format("x = {}\n", x));
  chance = round_rand(200.0 * atan((double)x) / 3.14159265);
  std::string long_msg = std::format(
      "{}/{}: {} [{}] tries insurgency vs {} [{}]\n\t{}: {} total civs [{}]  "
      "opposing {} total civs [{}]\n\t\t {} morale [{}] vs {} morale "
      "[{}]\n\t\t {} money against {} population at tax rate {}%\nSuccess "
      "chance is {}%\n",
      stars[g.snum].get_name(), stars[g.snum].get_planet_name(g.pnum),
      race.name, Playernum, alien.name, who, stars[g.snum].get_name(), eligible,
      Playernum, them, who, race.morale, Playernum, alien.morale, who, amount,
      p.info(who - 1).popn, p.info(who - 1).tax, chance);
  if (success(chance)) {
    changed_hands = revolt(p, who, Playernum);
    notify(Playernum, Governor, long_msg);
    notify(Playernum, Governor,
           std::format("Success!  You liberate {} sector{}.\n", changed_hands,
                       (changed_hands == 1) ? "" : "s"));
    long_msg += std::format(
        "A revolt on /{}/{} instigated by {} [{}] costs you {} sector{}\n",
        stars[g.snum].get_name(), stars[g.snum].get_planet_name(g.pnum),
        race.name, Playernum, changed_hands, (changed_hands == 1) ? "" : "s");
    warn(who, stars[g.snum].governor(who - 1), long_msg);
    p.info(Playernum - 1).tax = p.info(who - 1).tax;
    /* you inherit their tax rate (insurgency wars he he ) */
    post(std::format(
             "/{}/{}: Successful insurgency by {} [{}] against {} [{}]\n",
             stars[g.snum].get_name(), stars[g.snum].get_planet_name(g.pnum),
             race.name, Playernum, alien.name, who),
         NewsType::DECLARATION);
  } else {
    notify(Playernum, Governor, long_msg);
    g.out << "The insurgency failed!\n";
    long_msg += std::format("A revolt on /{}/{} instigated by {} [{}] fails\n",
                            stars[g.snum].get_name(),
                            stars[g.snum].get_planet_name(g.pnum), race.name,
                            Playernum);
    warn(who, stars[g.snum].governor(who - 1), long_msg);
    post(std::format("/{}/{}: Failed insurgency by {} [{}] against {} [{}]\n",
                     stars[g.snum].get_name(),
                     stars[g.snum].get_planet_name(g.pnum), race.name,
                     Playernum, alien.name, who),
         NewsType::DECLARATION);
  }
  deductAPs(g, APcount, g.snum);
  race.governor[Governor].money -= amount;
  putrace(race);
}
}  // namespace GB::commands
