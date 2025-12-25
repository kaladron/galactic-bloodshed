// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import scnlib;
import std;

module commands;

namespace GB::commands {
/*! Ship vs planet */
void bombard(const command_t& argv, GameObj& g) {
  int Playernum = g.player;
  int Governor = g.governor;
  ap_t APcount = 1;
  int x;
  int y;

  if (argv.size() < 2) {
    g.out << "Syntax: 'bombard <ship> [<x,y> [<strength>]]'.\n";
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& from = *ship_handle;

    if (!ship_matches_filter(argv[1], from)) continue;
    if (!authorized(Governor, from)) continue;
    if (!from.active()) {
      g.out << std::format("{} is irradiated and inactive.\n",
                           ship_to_string(from));
      continue;
    }

    if (from.whatorbits() != ScopeLevel::LEVEL_PLAN) {
      g.out << "You must be in orbit around a planet to bombard.\n";
      continue;
    }
    if (from.type() == ShipType::OTYPE_AFV && !landed(from)) {
      g.out << "This ship is not landed on the planet.\n";
      continue;
    }
    const auto* star = g.entity_manager.peek_star(from.storbits());
    if (!enufAP(Playernum, Governor, star->AP(Playernum - 1), APcount)) {
      continue;
    }

    auto maxstrength = check_retal_strength(from);

    int strength =
        (argv.size() > 3) ? std::stoi(argv[3]) : check_retal_strength(from);

    if (strength > maxstrength) {
      strength = maxstrength;
      g.out << std::format("{} set to {}\n",
                           laser_on(from) ? "Laser strength" : "Guns",
                           strength);
    }

    /* check to see if there is crystal overload */
    if (laser_on(from)) check_overload(g.entity_manager, from, 0, &strength);

    if (strength <= 0) {
      g.out << "No attack.\n";
      continue;
    }

    /* get planet using RAII for modifications */
    auto p_handle =
        g.entity_manager.get_planet(from.storbits(), from.pnumorbits());
    if (!p_handle.get()) {
      continue;
    }
    Planet& p = *p_handle;

    if (argv.size() > 2) {
      auto scan_result = scn::scan<int, int>(argv[2], "{},{}");
      if (!scan_result) {
        g.out << "Invalid sector format.\n";
        continue;
      }
      std::tie(x, y) = scan_result->values();
      if (x < 0 || x > p.Maxx() - 1 || y < 0 || y > p.Maxy() - 1) {
        g.out << "Illegal sector.\n";
        continue;
      }
    } else {
      x = int_rand(0, (int)p.Maxx() - 1);
      y = int_rand(0, (int)p.Maxy() - 1);
    }
    if (landed(from) && !adjacent(p, {from.land_x(), from.land_y()}, {x, y})) {
      g.out << "You are not adjacent to that sector.\n";
      continue;
    }

    bool has_defense =
        has_planet_defense(g.entity_manager, p.ships(), Playernum);

    if (has_defense && !landed(from)) {
      g.out << "Target has planetary defense networks.\n";
      g.out << "These have to be eliminated before you can attack sectors.\n";
      continue;
    }

    auto smap_handle =
        g.entity_manager.get_sectormap(from.storbits(), from.pnumorbits());
    if (!smap_handle.get()) {
      g.out << "Failed to load sector map.\n";
      continue;
    }
    SectorMap& smap = *smap_handle;
    char long_buf[1024], short_buf[256];
    auto result = shoot_ship_to_planet(g.entity_manager, from, p, strength, x,
                                       y, smap, 0, 0, long_buf, short_buf);

    if (result.numdest < 0) {
      g.out << "Illegal attack.\n";
      continue;
    }

    if (laser_on(from))
      use_fuel(from, 2.0 * (double)strength);
    else
      use_destruct(from, strength);

    post(g.entity_manager, short_buf, NewsType::COMBAT);
    notify_star(g.entity_manager, Playernum, Governor, from.storbits(),
                short_buf);
    for (auto i = 1; i <= g.entity_manager.num_races(); i++) {
      if (result.nuked[i - 1]) {
        const auto* star = g.entity_manager.peek_star(from.storbits());
        warn(i, star->governor(i - 1), long_buf);
      }
    }
    g.out << long_buf;

    if (DEFENSE) {
      /* planet retaliates - AFVs are immune to this */
      if (result.numdest && from.type() != ShipType::OTYPE_AFV) {
        for (auto i = 1; i <= g.entity_manager.num_races(); i++)
          if (result.nuked[i - 1] && !p.slaved_to()) {
            /* add planet defense strength */
            auto alien_handle = g.entity_manager.get_race(i);
            if (!alien_handle.get()) continue;
            Race& alien = *alien_handle;

            strength = MIN(p.info(i - 1).destruct, p.info(i - 1).guns);

            p.info(i - 1).destruct -= strength;

            shoot_planet_to_ship(g.entity_manager, alien, from, strength,
                                 long_buf, short_buf);
            const auto* star = g.entity_manager.peek_star(from.storbits());
            warn(i, star->governor(i - 1), long_buf);
            g.out << long_buf;
            if (!from.alive())
              post(g.entity_manager, short_buf, NewsType::COMBAT);
            notify_star(g.entity_manager, Playernum, Governor, from.storbits(),
                        short_buf);
          }
      }
    }

    /* protecting ships retaliate individually if damage was inflicted */
    /* AFVs are immune to this */
    if (result.numdest && from.alive() && from.type() != ShipType::OTYPE_AFV) {
      ShipList shiplist(g.entity_manager, p.ships());
      for (auto ship_handle : shiplist) {
        Ship& ship = *ship_handle;
        if (ship.protect().planet && ship.number() != from.number() &&
            ship.alive() && ship.active()) {
          if (laser_on(ship))
            check_overload(g.entity_manager, ship, 0, &strength);

          strength = check_retal_strength(ship);

          auto const& s2sresult =
              shoot_ship_to_ship(g.entity_manager, ship, from, strength, 0);
          if (s2sresult) {
            auto [_, short_buf, long_buf] = *s2sresult;

            if (laser_on(ship))
              use_fuel(ship, 2.0 * (double)strength);
            else
              use_destruct(ship, strength);
            if (!from.alive())
              post(g.entity_manager, short_buf, NewsType::COMBAT);
            notify_star(g.entity_manager, Playernum, Governor, from.storbits(),
                        short_buf);
            warn(ship.owner(), ship.governor(), long_buf);
            g.out << long_buf;
          }
        }
        if (!from.alive()) break;
      }
    }

    deductAPs(g, APcount, from.storbits());
  }  // end of ShipList iteration
}
}  // namespace GB::commands
