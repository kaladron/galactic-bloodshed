// SPDX-License-Identifier: Apache-2.0

/// \file bombard.cc
/// \brief Ship vs planet bombardment command.

module;

import session;
import gblib;
import notification;
import scnlib;
import std;

module commands;

namespace GB::commands {

/*! Ship vs planet */
bool bombard(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  bool any_fired = false;

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& from = *ship_handle;

    if (!ship_matches_filter(argv[1], from)) continue;
    if (!authorized(Governor, from)) continue;
    if (!from.active()) {
      g.out << std::format("{} is irradiated and inactive.\n", from);
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
    if (!g.deduct_ap(from.storbits(), 1)) {
      g.out << "You don't have 1 action points there.\n";
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

    g.entity_manager.mutate_planet(
        from.storbits(), from.pnumorbits(), [&](Planet& p) {
          bool has_defense =
              has_planet_defense(g.entity_manager, p.ships(), Playernum);

          if (has_defense && !landed(from)) {
            g.out << "Target has planetary defense networks.\n";
            g.out << "These have to be eliminated before you can attack "
                     "sectors.\n";
            return;
          }

          Coordinates target_coords;
          if (argv.size() > 2) {
            auto coords_opt = Coordinates::parse(argv[2]);
            if (!coords_opt) {
              g.out << "Invalid sector format.\n";
              return;
            }
            target_coords = *coords_opt;
            if (!p.is_valid(target_coords)) {
              g.out << "Illegal sector.\n";
              return;
            }
          } else {
            g.entity_manager.with_sectormap(
                from.storbits(), from.pnumorbits(), [&](const SectorMap& smap) {
                  target_coords = smap.get_random().coords();
                });
          }
          if (landed(from) && !adjacent(p, from.land_coords(), target_coords)) {
            g.out << "You are not adjacent to that sector.\n";
            return;
          }

          std::optional<std::tuple<int, std::array<char, MAXPLAYERS>,
                                   std::string, std::string>>
              opt_result;
          g.entity_manager.mutate_sectormap(
              from.storbits(), from.pnumorbits(), [&](SectorMap& smap) {
                opt_result =
                    shoot_ship_to_planet(g.entity_manager, from, p, strength,
                                         target_coords, smap, 0, 0);
              });

          if (!opt_result) {
            g.out << "Illegal attack.\n";
            return;
          }
          auto [numdest, nuked, short_msg, long_msg] = *opt_result;

          if (laser_on(from))
            use_fuel(from, 2.0 * (double)strength);
          else
            use_destruct(from, strength);

          post(g.entity_manager, short_msg, NewsType::COMBAT);
          notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                      from.storbits(), short_msg);
          for (auto i = 1; i <= g.entity_manager.num_races(); i++) {
            if (nuked[i - 1]) {
              const auto* star = g.entity_manager.peek_star(from.storbits());
              warn_player(g.session_registry, g.entity_manager, i,
                          star->governor(i), long_msg);
            }
          }
          g.out << long_msg;

          if (DEFENSE) {
            /* planet retaliates - AFVs are immune to this */
            if (numdest && from.type() != ShipType::OTYPE_AFV) {
              for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
                if (nuked[i.value - 1] && p.slaved_to() == 0) {
                  /* add planet defense strength */
                  g.entity_manager.mutate_race(i, [&](Race& alien) {
                    strength = MIN(p.info(i).destruct, p.info(i).guns);

                    p.info(i).destruct -= strength;

                    if (auto p2s_opt = shoot_planet_to_ship(
                            g.entity_manager, alien, from, strength)) {
                      auto [p_damage, p_short, p_long] = *p2s_opt;
                      const auto* star =
                          g.entity_manager.peek_star(from.storbits());
                      warn_player(g.session_registry, g.entity_manager, i,
                                  star->governor(i), p_long);
                      g.out << p_long;
                      if (!from.alive())
                        post(g.entity_manager, p_short, NewsType::COMBAT);
                      notify_star(g.session_registry, g.entity_manager,
                                  Playernum, Governor, from.storbits(),
                                  p_short);
                    }
                  });
                }
              }
            }
          }

          /* protecting ships retaliate individually if damage was inflicted */
          /* AFVs are immune to this */
          if (numdest && from.alive() && from.type() != ShipType::OTYPE_AFV) {
            ShipList shiplist(g.entity_manager, p.ships());
            for (auto ship_handle : shiplist) {
              Ship& ship = *ship_handle;
              if (ship.protect().planet && ship.number() != from.number() &&
                  ship.alive() && ship.active()) {
                if (laser_on(ship))
                  check_overload(g.entity_manager, ship, 0, &strength);

                strength = check_retal_strength(ship);

                auto const& s2sresult = shoot_ship_to_ship(
                    g.entity_manager, ship, from, strength, 0);
                if (s2sresult) {
                  auto [_, short_buf, long_buf] = *s2sresult;

                  if (laser_on(ship))
                    use_fuel(ship, 2.0 * (double)strength);
                  else
                    use_destruct(ship, strength);
                  if (!from.alive())
                    post(g.entity_manager, short_buf, NewsType::COMBAT);
                  notify_star(g.session_registry, g.entity_manager, Playernum,
                              Governor, from.storbits(), short_buf);
                  warn_player(g.session_registry, g.entity_manager,
                              ship.owner(), ship.governor(), long_buf);
                  g.out << long_buf;
                }
              }
              if (!from.alive()) break;
            }
          }
        });

    any_fired = true;
  }  // end of ShipList iteration

  return any_fired;
}

const CommandDescriptor bombard_cmd{
    .name = "bombard",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::planet_or_ship(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "bombard <ship> [<x,y> [<strength>]]",
    .description = "Bombard planetary sectors from orbiting or AFV ships",
    .handler = &bombard,
};

}  // namespace GB::commands
