// SPDX-License-Identifier: Apache-2.0

module;

import session;
import gb.entities;
import gb.services;
import notification;
import scnlib;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool insurgency(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  player_t who{0};
  int eligible;
  int them = 0;
  double x;
  int changed_hands;
  int chance;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must 'cs' to the planet you wish to try it on.\n";
    return false;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return false;
  }

  who = get_player(g.entity_manager, argv[1]);
  if (who.value == 0) {
    g.out << "No such player.\n";
    return false;
  }
  const auto* alien = g.entity_manager.peek_race(who);
  if (alien->Guest) {
    g.out << "Don't be such a dickweed.\n";
    return false;
  }
  if (who == Playernum) {
    g.out << "You can't revolt against yourself!\n";
    return false;
  }
  eligible = 0;
  them = 0;
  PlanetList planets(g.entity_manager, g.snum(), star);
  for (auto planet_handle : planets) {
    eligible += planet_handle->info(Playernum).popn;
    them += planet_handle->info(who).popn;
  }
  if (!eligible) {
    g.out << "You must have population in the star system to attempt "
             "insurgency\n.";
    return false;
  }
  bool ok = false;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    if (!p.info(who).popn) {
      g.out << "This player does not occupy this planet.\n";
      return;
    }

    int amount = std::stoi(argv[2]);
    if (amount < 0) {
      g.out << "You have to use a positive amount of money.\n";
      return;
    }
    if (g.race->governor[Governor.value].money < amount) {
      g.out << "Nice try.\n";
      return;
    }

    x = INSURG_FACTOR * (double)amount * (double)p.info(who).tax /
        (double)p.info(who).popn;
    x *= morale_factor((double)(g.race->morale - alien->morale));
    x *= morale_factor((double)(eligible - them) / 50.0);
    x *= morale_factor(10.0 *
                       (double)(g.race->fighters * p.info(Playernum).troops -
                                alien->fighters * p.info(who).troops)) /
         50.0;
    g.out << std::format("x = {}\n", x);
    chance = round_rand(200.0 * std::atan((double)x) / 3.14159265);
    std::string long_msg = std::format(
        "{}/{}: {} [{}] tries insurgency vs {} [{}]\n\t{}: {} total civs [{}]  "
        "opposing {} total civs [{}]\n\t\t {} morale [{}] vs {} morale "
        "[{}]\n\t\t {} money against {} population at tax rate {}%\nSuccess "
        "chance is {}%\n",
        star.get_name(), star.get_planet_name(g.pnum()), g.race->name,
        Playernum, alien->name, who, star.get_name(), eligible, Playernum, them,
        who, g.race->morale, Playernum, alien->morale, who, amount,
        p.info(who).popn, p.info(who).tax, chance);
    if (success(chance)) {
      changed_hands =
          revolt(p, g.entity_manager, g.snum(), g.pnum(), who, Playernum);
      g.out << long_msg;
      g.out << std::format("Success!  You liberate {} sector{}.\n",
                           changed_hands, (changed_hands == 1) ? "" : "s");
      long_msg += std::format(
          "A revolt on /{}/{} instigated by {} [{}] costs you {} sector{}\n",
          star.get_name(), star.get_planet_name(g.pnum()), g.race->name,
          Playernum, changed_hands, (changed_hands == 1) ? "" : "s");
      warn_player(g.session_registry, g.entity_manager, who, star.governor(who),
                  long_msg);
      p.info(Playernum).tax = p.info(who).tax;
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
      long_msg += std::format(
          "A revolt on /{}/{} instigated by {} [{}] fails\n", star.get_name(),
          star.get_planet_name(g.pnum()), g.race->name, Playernum);
      warn_player(g.session_registry, g.entity_manager, who, star.governor(who),
                  long_msg);
      post(g.entity_manager,
           std::format("/{}/{}: Failed insurgency by {} [{}] against {} [{}]\n",
                       star.get_name(), star.get_planet_name(g.pnum()),
                       g.race->name, Playernum, alien->name, who),
           NewsType::DECLARATION);
    }
    // Need mutable access for money deduction
    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      race.governor[Governor.value].money -= amount;
    });
    ok = true;
  });
  return ok;
}

const CommandDescriptor insurgency_cmd{
    .name = "insurgency",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(10),
    .min_args = 3,
    .syntax = "insurgency <race> <money>",
    .description = "Finance a planetary insurgency against an occupying player",
    .handler = &insurgency,
};

}  // namespace GB::commands
