// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include <strings.h>

module commands;

namespace GB::commands {
/*! Planet vs ship */
void defend(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  int sh;
  Ship* ship;
  Ship dummy;
  int strength;
  int retal;
  int damage;
  int x;
  int y;
  int numdest;

  if (!DEFENSE) return;

  /* for telegramming and retaliating */
  Nuked.fill(0);

  /* get the planet from the players current scope */
  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to set scope to the planet first.\n";
    return;
  }

  if (argv.size() < 3) {
    notify(Playernum, Governor,
           "Syntax: 'defend <ship> <sector> [<strength>]'.\n");
    return;
  }
  if (Governor && stars[g.snum].governor(Playernum - 1) != Governor) {
    notify(Playernum, Governor,
           "You are not authorized to do that in this system.\n");
    return;
  }
  auto toshiptmp = string_to_shipnum(argv[1]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return;
  }
  auto toship = *toshiptmp;

  if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1), APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (!p.info(Playernum - 1).numsectsowned) {
    g.out << "You do not occupy any sectors here.\n";
    return;
  }

  if (p.slaved_to() && p.slaved_to() != Playernum) {
    g.out << "This planet is enslaved.\n";
    return;
  }

  auto to = getship(toship);
  if (!to) {
    return;
  }

  if (to->whatorbits != ScopeLevel::LEVEL_PLAN) {
    g.out << "The ship is not in planet orbit.\n";
    return;
  }

  if (to->storbits != g.snum || to->pnumorbits != g.pnum) {
    g.out << "Target is not in orbit around this planet.\n";
    return;
  }

  if (landed(*to)) {
    g.out << "Planet guns can't fire on landed ships.\n";
    return;
  }

  /* save defense strength for retaliation */
  retal = check_retal_strength(*to);
  bcopy(&*to, &dummy, sizeof(Ship));

  sscanf(argv[2].c_str(), "%d,%d", &x, &y);

  if (x < 0 || x > p.Maxx() - 1 || y < 0 || y > p.Maxy() - 1) {
    g.out << "Illegal sector.\n";
    return;
  }

  /* check to see if you own the sector */
  auto sect = getsector(p, x, y);
  if (sect.get_owner() != Playernum) {
    g.out << "Nice try.\n";
    return;
  }

  if (argv.size() >= 4)
    strength = std::stoi(argv[3]);
  else
    strength = p.info(Playernum - 1).guns;

  strength = MIN(strength, p.info(Playernum - 1).destruct);
  strength = MIN(strength, p.info(Playernum - 1).guns);

  if (strength <= 0) {
    g.out << std::format("No attack - {} guns, {}d\n",
                         p.info(Playernum - 1).guns,
                         p.info(Playernum - 1).destruct);
    return;
  }
  auto& race = races[Playernum - 1];

  char long_buf[1024], short_buf[256];
  damage = shoot_planet_to_ship(race, *to, strength, long_buf, short_buf);

  if (!to->alive && to->type == ShipType::OTYPE_TOXWC) {
    /* get planet again since toxicity probably has changed */
    p = getplanet(g.snum, g.pnum);
  }

  if (damage < 0) {
    g.out << std::format("Target out of range  {}!\n", SYSTEMSIZE);
    return;
  }

  p.info(Playernum - 1).destruct -= strength;
  if (!to->alive) post(short_buf, NewsType::COMBAT);
  notify_star(Playernum, Governor, to->storbits, short_buf);
  warn(to->owner, to->governor, long_buf);
  notify(Playernum, Governor, long_buf);

  /* defending ship retaliates */

  strength = 0;
  if (retal && damage && to->protect.self) {
    strength = retal;
    if (laser_on(*to)) check_overload(*to, 0, &strength);

    auto smap = getsmap(p);
    if ((numdest = shoot_ship_to_planet(dummy, p, strength, x, y, smap, 0, 0,
                                        long_buf, short_buf)) >= 0) {
      if (laser_on(*to))
        use_fuel(*to, 2.0 * (double)strength);
      else
        use_destruct(*to, strength);

      post(short_buf, NewsType::COMBAT);
      notify_star(Playernum, Governor, to->storbits, short_buf);
      notify(Playernum, Governor, long_buf);
      warn(to->owner, to->governor, long_buf);
    }
    putsmap(smap, p);
  }

  /* protecting ships retaliate individually if damage was inflicted */
  if (damage) {
    sh = p.ships();
    while (sh) {
      (void)getship(&ship, sh);
      if (ship->protect.on && (ship->protect.ship == toship) &&
          (ship->protect.ship == toship) && sh != toship && ship->alive &&
          ship->active) {
        if (laser_on(*ship)) check_overload(*ship, 0, &strength);
        strength = check_retal_strength(*ship);

        auto smap = getsmap(p);
        if ((numdest = shoot_ship_to_planet(*ship, p, strength, x, y, smap, 0,
                                            0, long_buf, short_buf)) >= 0) {
          if (laser_on(*ship))
            use_fuel(*ship, 2.0 * (double)strength);
          else
            use_destruct(*ship, strength);
          post(short_buf, NewsType::COMBAT);
          notify_star(Playernum, Governor, ship->storbits, short_buf);
          notify(Playernum, Governor, long_buf);
          warn(ship->owner, ship->governor, long_buf);
        }
        putsmap(smap, p);
        putship(*ship);
      }
      sh = ship->nextship;
      free(ship);
    }
  }

  /* write the ship stuff out to disk */
  putship(*to);
  putplanet(p, stars[g.snum], g.pnum);

  deductAPs(g, APcount, g.snum);
}
}  // namespace GB::commands
