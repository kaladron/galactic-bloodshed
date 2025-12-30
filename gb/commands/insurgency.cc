// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import scnlib;
import std;

module commands;

namespace GB::commands {
void insurgency(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 10;
  int who;
  int eligible;
  int them = 0;
  double x;
  int changed_hands;
  int chance;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must 'cs' to the planet you wish to try it on.\n";
    return;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  /*  if(argv.size()<3) {
        g.out << "The correct syntax is 'insurgency <race>
    <money>'\n";
        return;
    }*/
  if (!enufAP(Playernum, Governor, star.AP(Playernum - 1), APcount)) return;
  if (!(who = get_player(g.entity_manager, argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  const auto* alien = g.entity_manager.peek_race(who);
  if (alien->Guest) {
    g.out << "Don't be such a dickweed.\n";
    return;
  }
  if (who == Playernum) {
    g.out << "You can't revolt against yourself!\n";
    return;
  }
  eligible = 0;
  them = 0;
  PlanetList planets(g.entity_manager, g.snum(), star);
  for (auto planet_handle : planets) {
    eligible += planet_handle->info(Playernum - 1).popn;
    them += planet_handle->info(who - 1).popn;
  }
  if (!eligible) {
    g.out << "You must have population in the star system to attempt "
             "insurgency\n.";
    return;
  }
  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& p = *planet_handle;

  if (!p.info(who - 1).popn) {
    g.out << "This player does not occupy this planet.\n";
    return;
  }

  int amount = std::stoi(argv[2]);
  if (amount < 0) {
    g.out << "You have to use a positive amount of money.\n";
    return;
  }
  if (g.race->governor[Governor].money < amount) {
    g.out << "Nice try.\n";
    return;
  }

  x = INSURG_FACTOR * (double)amount * (double)p.info(who - 1).tax /
      (double)p.info(who - 1).popn;
  x *= morale_factor((double)(g.race->morale - alien->morale));
  x *= morale_factor((double)(eligible - them) / 50.0);
  x *= morale_factor(10.0 *
                     (double)(g.race->fighters * p.info(Playernum - 1).troops -
                              alien->fighters * p.info(who - 1).troops)) /
       50.0;
  g.out << std::format("x = {}\n", x);
  chance = round_rand(200.0 * std::atan((double)x) / 3.14159265);
  std::string long_msg = std::format(
      "{}/{}: {} [{}] tries insurgency vs {} [{}]\n\t{}: {} total civs [{}]  "
      "opposing {} total civs [{}]\n\t\t {} morale [{}] vs {} morale "
      "[{}]\n\t\t {} money against {} population at tax rate {}%\nSuccess "
      "chance is {}%\n",
      star.get_name(), star.get_planet_name(g.pnum()), g.race->name, Playernum,
      alien->name, who, star.get_name(), eligible, Playernum, them, who,
      g.race->morale, Playernum, alien->morale, who, amount,
      p.info(who - 1).popn, p.info(who - 1).tax, chance);
  if (success(chance)) {
    changed_hands =
        revolt(p, g.entity_manager, g.snum(), g.pnum(), who, Playernum);
    g.out << long_msg;
    g.out << std::format("Success!  You liberate {} sector{}.\n", changed_hands,
                         (changed_hands == 1) ? "" : "s");
    long_msg += std::format(
        "A revolt on /{}/{} instigated by {} [{}] costs you {} sector{}\n",
        star.get_name(), star.get_planet_name(g.pnum()), g.race->name,
        Playernum, changed_hands, (changed_hands == 1) ? "" : "s");
    warn(who, star.governor(who - 1), long_msg);
    p.info(Playernum - 1).tax = p.info(who - 1).tax;
    /* you inherit their tax rate (insurgency wars he he ) */
    post(g.entity_manager,
         std::format(
             "/{}/{}: Successful insurgency by {} [{}] against {} [{}]\n",
             star.get_name(), star.get_planet_name(g.pnum()), g.race->name,
             Playernum, alien->name, who),
         NewsType::DECLARATION);
  } else {
    g.out << long_msg;
    g.out << "The insurgency failed!\n";
    long_msg += std::format("A revolt on /{}/{} instigated by {} [{}] fails\n",
                            star.get_name(), star.get_planet_name(g.pnum()),
                            g.race->name, Playernum);
    warn(who, star.governor(who - 1), long_msg);
    post(g.entity_manager,
         std::format("/{}/{}: Failed insurgency by {} [{}] against {} [{}]\n",
                     star.get_name(), star.get_planet_name(g.pnum()),
                     g.race->name, Playernum, alien->name, who),
         NewsType::DECLARATION);
  }
  deductAPs(g, APcount, g.snum());

  // Need mutable access for money deduction
  auto race_handle = g.entity_manager.get_race(Playernum);
  race_handle->governor[Governor].money -= amount;
}
}  // namespace GB::commands
