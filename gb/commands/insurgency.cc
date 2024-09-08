// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace GB::commands {
void insurgency(const command_t &argv, GameObj &g) {
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
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  /*  if(argv.size()<3) {
        notify(Playernum, Governor, "The correct syntax is 'insurgency <race>
    <money>'\n");
        return;
    }*/
  if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcount))
    return;
  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  auto &race = races[Playernum - 1];
  auto &alien = races[who - 1];
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
  for (i = 0; i < stars[g.snum].numplanets; i++) {
    auto p = getplanet(g.snum, i);
    eligible += p.info[Playernum - 1].popn;
    them += p.info[who - 1].popn;
  }
  if (!eligible) {
    g.out << "You must have population in the star system to attempt "
             "insurgency\n.";
    return;
  }
  auto p = getplanet(g.snum, g.pnum);

  if (!p.info[who - 1].popn) {
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

  x = INSURG_FACTOR * (double)amount * (double)p.info[who - 1].tax /
      (double)p.info[who - 1].popn;
  x *= morale_factor((double)(race.morale - alien.morale));
  x *= morale_factor((double)(eligible - them) / 50.0);
  x *= morale_factor(10.0 *
                     (double)(race.fighters * p.info[Playernum - 1].troops -
                              alien.fighters * p.info[who - 1].troops)) /
       50.0;
  sprintf(buf, "x = %f\n", x);
  notify(Playernum, Governor, buf);
  chance = round_rand(200.0 * atan((double)x) / 3.14159265);
  char long_buf[1024];
  sprintf(long_buf, "%s/%s: %s [%d] tries insurgency vs %s [%d]\n",
          stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
          Playernum, alien.name, who);
  sprintf(buf, "\t%s: %d total civs [%d]  opposing %d total civs [%d]\n",
          stars[g.snum].name, eligible, Playernum, them, who);
  strcat(long_buf, buf);
  sprintf(buf, "\t\t %ld morale [%d] vs %ld morale [%d]\n", race.morale,
          Playernum, alien.morale, who);
  strcat(long_buf, buf);
  sprintf(buf, "\t\t %d money against %ld population at tax rate %d%%\n",
          amount, p.info[who - 1].popn, p.info[who - 1].tax);
  strcat(long_buf, buf);
  sprintf(buf, "Success chance is %d%%\n", chance);
  strcat(long_buf, buf);
  if (success(chance)) {
    changed_hands = revolt(p, who, Playernum);
    notify(Playernum, Governor, long_buf);
    sprintf(buf, "Success!  You liberate %d sector%s.\n", changed_hands,
            (changed_hands == 1) ? "" : "s");
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "A revolt on /%s/%s instigated by %s [%d] costs you %d sector%s\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
            Playernum, changed_hands, (changed_hands == 1) ? "" : "s");
    strcat(long_buf, buf);
    warn(who, stars[g.snum].governor[who - 1], long_buf);
    p.info[Playernum - 1].tax = p.info[who - 1].tax;
    /* you inherit their tax rate (insurgency wars he he ) */
    sprintf(buf, "/%s/%s: Successful insurgency by %s [%d] against %s [%d]\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
            Playernum, alien.name, who);
    post(buf, NewsType::DECLARATION);
  } else {
    notify(Playernum, Governor, long_buf);
    g.out << "The insurgency failed!\n";
    sprintf(buf, "A revolt on /%s/%s instigated by %s [%d] fails\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
            Playernum);
    strcat(long_buf, buf);
    warn(who, stars[g.snum].governor[who - 1], long_buf);
    sprintf(buf, "/%s/%s: Failed insurgency by %s [%d] against %s [%d]\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
            Playernum, alien.name, who);
    post(buf, NewsType::DECLARATION);
  }
  deductAPs(g, APcount, g.snum);
  race.governor[Governor].money -= amount;
  putrace(race);
}
}  // namespace GB::commands
