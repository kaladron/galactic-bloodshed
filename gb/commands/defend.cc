// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import notification;
import scnlib;
import session;
import std;

module commands;

namespace GB::commands {
/*! Planet vs ship */
bool defend(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 1;
  int strength;
  int retal;
  int damage;

  if (!DEFENSE) return false;

  /* get the planet from the players current scope */
  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to set scope to the planet first.\n";
    return false;
  }

  if (argv.size() < 3) {
    g.out << "Syntax: 'defend <ship> <sector> [<strength>]'.\n";
    return false;
  }
  bool auth = g.entity_manager.with_star(g.snum(), [&](const Star& star) {
    return (Governor == 0 || star.governor(Playernum) == Governor);
  });
  if (!auth) {
    g.out << "You are not authorized to do that in this system.\n";
    return false;
  }
  auto toshiptmp = string_to_shipnum(argv[1]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }
  auto toship = *toshiptmp;

  if (!g.deduct_ap(g.snum(), APcount)) {
    g.out << "You don't have enough action points.\n";
    return false;
  }

  bool valid_target = false;
  try {
    valid_target = g.entity_manager.with_ship(toship, [&](const Ship& to) {
      if (to.whatorbits() != ScopeLevel::LEVEL_PLAN) {
        g.out << "The ship is not in planet orbit.\n";
        return false;
      }
      if (to.storbits() != g.snum() || to.pnumorbits() != g.pnum()) {
        g.out << "Target is not in orbit around this planet.\n";
        return false;
      }
      if (landed(to)) {
        g.out << "Planet guns can't fire on landed ships.\n";
        return false;
      }
      /* save defense attack strength for retaliation */
      // Calculate retaliation strength BEFORE damage is applied.
      // This pre-damage strength will be used if the target retaliates,
      // even though the ship itself will be modified by taking damage.
      retal = check_retal_strength(to);
      return true;
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return false;
  }
  if (!valid_target) return false;

  auto coords_opt = Coordinates::parse(argv[2]);
  if (!coords_opt) {
    g.out << "Bad format for sector.\n";
    return false;
  }
  const Coordinates sector_coords = *coords_opt;

  bool valid_planet =
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        if (!p.info(Playernum).numsectsowned) {
          g.out << "You do not occupy any sectors here.\n";
          return false;
        }
        if (p.slaved_to() != 0 && p.slaved_to() != Playernum) {
          g.out << "This planet is enslaved.\n";
          return false;
        }
        if (!p.is_valid(sector_coords)) {
          g.out << "Illegal sector.\n";
          return false;
        }
        return true;
      });
  if (!valid_planet) return false;

  /* check to see if you own the sector */
  bool owned = g.entity_manager.with_sectormap(
      g.snum(), g.pnum(), [&](const SectorMap& smap) {
        return smap.get(sector_coords).get_owner() == Playernum;
      });
  if (!owned) {
    g.out << "Nice try.\n";
    return false;
  }

  if (argv.size() >= 4) {
    strength = std::stoi(argv[3]);
  } else {
    strength =
        g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
          return p.info(Playernum).guns;
        });
  }

  bool can_attack =
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        strength = MIN(strength, p.info(Playernum).destruct);
        strength = MIN(strength, p.info(Playernum).guns);
        if (strength <= 0) {
          g.out << std::format("No attack - {} guns, {}d\n",
                               p.info(Playernum).guns,
                               p.info(Playernum).destruct);
          return false;
        }
        return true;
      });
  if (!can_attack) return false;

  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    g.entity_manager.mutate_ship(toship, [&](Ship& target_ship) {
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
        g.entity_manager.mutate_sectormap(
            g.snum(), g.pnum(), [&](SectorMap& smap) {
              auto p2s_opt = shoot_planet_to_ship(g.entity_manager, race,
                                                  target_ship, strength);
              if (!p2s_opt) {
                g.out << std::format("Target out of range  {}!\n", SYSTEMSIZE);
                return;
              }
              auto [p_damage, p_short, p_long] = *p2s_opt;
              damage = p_damage;

              p.info(Playernum).destruct -= strength;
              if (!target_ship.alive())
                post(g.entity_manager, p_short, NewsType::COMBAT);
              notify_star(g.session_registry, g.entity_manager, Playernum,
                          Governor, target_ship.storbits(), p_short);
              warn_player(g.session_registry, g.entity_manager,
                          target_ship.owner(), target_ship.governor(), p_long);
              g.out << p_long;

              /* defending ship retaliates */
              strength = 0;
              if (retal && damage && target_ship.protect().self) {
                // Use pre-damage retaliation strength (saved in 'retal' above).
                // shoot_ship_to_planet() uses the explicit strength parameter,
                // not the ship's current damage state, so this correctly
                // applies the ship's original (pre-damage) attack capability.
                strength = retal;
                if (laser_on(target_ship))
                  check_overload(g.entity_manager, target_ship, 0, &strength);

                if (auto result_opt = shoot_ship_to_planet(
                        g.entity_manager, target_ship, p, strength,
                        sector_coords, smap, 0, 0)) {
                  auto [_, __, short_msg, long_msg] = *result_opt;
                  if (laser_on(target_ship))
                    use_fuel(target_ship, 2.0 * (double)strength);
                  else
                    use_destruct(target_ship, strength);

                  post(g.entity_manager, short_msg, NewsType::COMBAT);
                  notify_star(g.session_registry, g.entity_manager, Playernum,
                              Governor, target_ship.storbits(), short_msg);
                  g.out << long_msg;
                  warn_player(g.session_registry, g.entity_manager,
                              target_ship.owner(), target_ship.governor(),
                              long_msg);
                }
              }

              /* protecting ships retaliate individually if damage was inflicted
               */
              if (damage) {
                const ShipList shiplist(g.entity_manager, p.ships());
                for (const Ship& ship : shiplist) {
                  if (ship.protect().on && (ship.protect().ship == toship) &&
                      ship.number() != toship && ship.alive() &&
                      ship.active()) {
                    strength = check_retal_strength(ship);
                    if (laser_on(ship))
                      check_overload(g.entity_manager, const_cast<Ship&>(ship),
                                     0, &strength);

                    if (auto result2_opt = shoot_ship_to_planet(
                            g.entity_manager, ship, p, strength, sector_coords,
                            smap, 0, 0)) {
                      auto [_, __, short_msg2, long_msg2] = *result2_opt;
                      g.entity_manager.mutate_ship(
                          ship.number(), [&](Ship& ship_mut) {
                            if (laser_on(ship))
                              use_fuel(ship_mut, 2.0 * (double)strength);
                            else
                              use_destruct(ship_mut, strength);
                          });
                      post(g.entity_manager, short_msg2, NewsType::COMBAT);
                      notify_star(g.session_registry, g.entity_manager,
                                  Playernum, Governor, ship.storbits(),
                                  short_msg2);
                      g.out << long_msg2;
                      warn_player(g.session_registry, g.entity_manager,
                                  ship.owner(), ship.governor(), long_msg2);
                    }
                  }
                }
              }
            });
      });
    });
  });

  return true;
}

const CommandDescriptor defend_cmd{
    .name = "defend",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "defend <ship> <sector> [<strength>]",
    .description =
        "Defend planet against orbiting ships using planetary defense guns",
    .handler = &defend,
};

}  // namespace GB::commands
