// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include <strings.h>

module commands;

namespace GB::commands {
/*! Ship vs ship */
void fire(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount;
  int cew;
  // This is called from dock.cc.
  if (argv[0] == "fire-from-dock") {
    // TODO(jeffbailey): It's not clear that cew is ever used as anything other
    // than a true/false value.
    cew = 3;
    APcount = 0;
  } else if (argv[0] == "cew") {
    cew = 1;
    APcount = 1;
  } else {  // argv[0] = fire
    cew = 0;
    APcount = 1;
  }
  shipnum_t toship;
  shipnum_t sh;
  Ship dummy;
  int strength;
  int maxstrength;
  int retal;

  sh = 0;  // TODO(jeffbailey): No idea what this is, init to 0.

  /* for telegramming and retaliating */
  Nuked.fill(0);

  if (argv.size() < 3) {
    std::string msg =
        "Syntax: '" + argv[0] + " <ship> <target> [<strength>]'.\n";
    notify(Playernum, Governor, msg);
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& from = *ship_handle;

    if (!ship_matches_filter(argv[1], from)) continue;
    if (!authorized(Governor, from)) continue;
    if (!from.active) {
      notify(Playernum, Governor,
             std::format("{} is irradiated and inactive.\n",
                         ship_to_string(from)));
      continue;
    }
    if (from.whatorbits == ScopeLevel::LEVEL_UNIV) {
      if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
        continue;
      }
    } else if (!enufAP(Playernum, Governor,
                       stars[from.storbits].AP(Playernum - 1), APcount)) {
      continue;
    }
    if (cew) {
      if (!from.cew) {
        notify(Playernum, Governor,
               "That ship is not equipped to fire CEWs.\n");
        continue;
      }
      if (!from.mounted) {
        notify(Playernum, Governor,
               "You need to have a crystal mounted to fire CEWs.\n");
        continue;
      }
    }
    auto toshiptmp = string_to_shipnum(argv[1]);
    if (!toshiptmp || *toshiptmp <= 0) {
      g.out << "Bad ship number.\n";
      return;
    }
    toship = *toshiptmp;
    if (toship == from.number) {
      g.out << "Get real.\n";
      continue;
    }
    auto to = getship(toship);
    if (!to) {
      continue;
    }

    /* save defense attack strength for retaliation */
    retal = check_retal_strength(*to);
    bcopy(&*to, &dummy, sizeof(Ship));

    if (from.type == ShipType::OTYPE_AFV) {
      if (!landed(from)) {
        notify(Playernum, Governor,
               std::format("{} isn't landed on a planet!\n",
                           ship_to_string(from)));
        continue;
      }
      if (!landed(*to)) {
        notify(
            Playernum, Governor,
            std::format("{} isn't landed on a planet!\n", ship_to_string(*to)));
        continue;
      }
    }
    if (landed(from) && landed(*to)) {
      if ((from.storbits != to->storbits) ||
          (from.pnumorbits != to->pnumorbits)) {
        notify(Playernum, Governor,
               "Landed ships can only attack other "
               "landed ships if they are on the same "
               "planet!\n");
        continue;
      }
      const auto p = getplanet(from.storbits, from.pnumorbits);
      if (!adjacent(p, {from.land_x, from.land_y}, {to->land_x, to->land_y})) {
        g.out << "You are not adjacent to your target!\n";
        continue;
      }
    }
    if (cew) {
      if (from.fuel < (double)from.cew) {
        notify(Playernum, Governor,
               std::format("You need {} fuel to fire CEWs.\n", from.cew));
        continue;
      }
      if (landed(from) || landed(*to)) {
        notify(Playernum, Governor,
               "CEWs cannot originate from or targeted "
               "to ships landed on planets.\n");
        continue;
      }
      notify(Playernum, Governor, std::format("CEW strength {}.\n", from.cew));
      strength = from.cew / 2;

    } else {
      maxstrength = check_retal_strength(from);

      if (argv.size() >= 4)
        strength = std::stoi(argv[3]);
      else
        strength = check_retal_strength(from);

      if (strength > maxstrength) {
        strength = maxstrength;
        notify(Playernum, Governor,
               std::format("{} set to {}\n",
                           (laser_on(from) ? "Laser strength" : "Guns"),
                           strength));
      }
    }

    /* check to see if there is crystal overloads */
    if (laser_on(from) || cew) check_overload(from, cew, &strength);

    if (strength <= 0) {
      notify(Playernum, Governor, "No attack.\n");
      continue;
    }

    auto s2sresult = shoot_ship_to_ship(from, *to, strength, cew);

    if (!s2sresult) {
      g.out << "Illegal attack.\n";
      continue;
    }

    auto const& [damage, short_buf, long_buf] = *s2sresult;

    if (laser_on(from) || cew)
      use_fuel(from, 2.0 * (double)strength);
    else
      use_destruct(from, strength);

    if (!to->alive) post(short_buf, NewsType::COMBAT);
    notify_star(Playernum, Governor, from.storbits, short_buf);
    warn(to->owner, to->governor, long_buf);
    notify(Playernum, Governor, long_buf);
    /* defending ship retaliates */

    strength = 0;
    if (retal && damage && to->protect.self) {
      strength = retal;
      if (laser_on(*to)) check_overload(*to, 0, &strength);

      auto s2sresult = shoot_ship_to_ship(dummy, from, strength, 0, true);
      if (s2sresult) {
        auto const& [damage, short_buf, long_buf] = *s2sresult;

        if (laser_on(*to))
          use_fuel(*to, 2.0 * (double)strength);
        else
          use_destruct(*to, strength);
        if (!from.alive) post(short_buf, NewsType::COMBAT);
        notify_star(Playernum, Governor, from.storbits, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(to->owner, to->governor, long_buf);
      }
    }
    /* protecting ships retaliate individually if damage was inflicted */
    /* AFVs immune to retaliation of this type */
    if (damage && from.alive && from.type != ShipType::OTYPE_AFV) {
      if (to->whatorbits == ScopeLevel::LEVEL_STAR) /* star level ships */
        sh = stars[to->storbits].ships();
      if (to->whatorbits == ScopeLevel::LEVEL_PLAN) { /* planet level ships */
        const auto p = getplanet(to->storbits, to->pnumorbits);
        sh = p.ships();
      }
      ShipList shiplist(g.entity_manager, sh);
      for (auto ship_handle : shiplist) {
        if (!from.alive) break;
        Ship& ship = *ship_handle;
        if (ship.protect.on && (ship.protect.ship == toship) &&
            (ship.protect.ship == toship) && ship.number != from.number &&
            ship.number != toship && ship.alive && ship.active) {
          strength = check_retal_strength(ship);
          if (laser_on(ship)) check_overload(ship, 0, &strength);

          auto s2sresult = shoot_ship_to_ship(ship, from, strength, 0);
          if (s2sresult) {
            auto const& [damange, short_buf, long_buf] = *s2sresult;
            if (laser_on(ship))
              use_fuel(ship, 2.0 * (double)strength);
            else
              use_destruct(ship, strength);
            if (!from.alive) post(short_buf, NewsType::COMBAT);
            notify_star(Playernum, Governor, from.storbits, short_buf);
            notify(Playernum, Governor, long_buf);
            warn(ship.owner, ship.governor, long_buf);
          }
        }
      }
    }
    putship(*to);
    deductAPs(g, APcount, from.storbits);
  }  // end of ShipList iteration
}
}  // namespace GB::commands
