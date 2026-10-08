// SPDX-License-Identifier: Apache-2.0

module;

import gb.entities;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {
/*! Planet vs ship */
bool defend(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  weapon_power_t strength;
  weapon_power_t retal;
  damage_t damage;

  if (!DEFENSE) return false;

  auto toshiptmp = string_to_shipnum(argv[1]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }
  auto toship = *toshiptmp;

  bool valid_target = false;
  try {
    valid_target = g.entity_manager.with_ship(toship, [&](const Ship& to) {
      if (!to.alive()) {
        g.out << "That ship is already destroyed.\n";
        return false;
      }
      if (to.whatorbits() != ScopeLevel::LEVEL_PLAN) {
        g.out << "The ship is not in planet orbit.\n";
        return false;
      }
      if (to.storbits() != g.snum() || to.pnumorbits() != g.pnum()) {
        g.out << "Target is not in orbit around this planet.\n";
        return false;
      }
      if (to.is_landed()) {
        g.out << "Planet guns can't fire on landed ships.\n";
        return false;
      }
      /* save defense attack strength for retaliation */
      // Calculate retaliation strength BEFORE damage is applied.
      // This pre-damage strength will be used if the target retaliates,
      // even though the ship itself will be modified by taking damage.
      retal = to.check_retal_strength();
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
        if (p.is_enslaved_to_foreign(Playernum)) {
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
    int parsed_strength = std::stoi(argv[3]);
    strength =
        parsed_strength > 0 ? static_cast<weapon_power_t>(parsed_strength) : 0;
  } else {
    strength =
        g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
          return static_cast<weapon_power_t>(p.info(Playernum).guns);
        });
  }

  bool can_attack =
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        strength =
            std::min(strength, static_cast<weapon_power_t>(std::max<resource_t>(
                                   0, p.info(Playernum).destruct)));
        strength = std::min(
            strength, static_cast<weapon_power_t>(p.info(Playernum).guns));
        if (strength <= 0) {
          g.out << std::format("No attack - {} guns, {}d\n",
                               p.info(Playernum).guns,
                               p.info(Playernum).destruct);
          return false;
        }
        return true;
      });
  if (!can_attack) return false;

  bool fired = false;
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
              fired = true;
              damage = p2s_opt->damage;
              const std::string p_short =
                  GB::presentation::render_ship_shot_short(*p2s_opt);
              const std::string p_long =
                  GB::presentation::render_ship_shot_long(*p2s_opt);

              p.info(Playernum).destruct -= strength;
              if (!target_ship.alive())
                post(g.entity_manager, p_short, NewsType::COMBAT);
              notify_star(g.session_registry, g.entity_manager, Playernum,
                          Governor, target_ship.storbits(), p_short);
              warn_player(g.session_registry, g.entity_manager,
                          target_ship.owner(), target_ship.governor(), p_long);
              g.present(*p2s_opt);

              /* defending ship retaliates */
              if (retal && damage && target_ship.protect().retaliate) {
                // Use pre-damage retaliation strength (saved in 'retal' above).
                // shoot_ship_to_planet() uses the explicit strength parameter,
                // not the ship's current damage state, so this correctly
                // applies the ship's original (pre-damage) attack capability.
                auto [retal_strength, overload] =
                    check_overload(g.entity_manager, target_ship, 0, retal);
                if (overload) {
                  notify_reactor_overload(g.entity_manager, *overload);
                } else if (auto result_opt = shoot_ship_to_planet(
                               g.entity_manager, target_ship, p, retal_strength,
                               sector_coords, smap, false, guntype_t::NONE)) {
                  target_ship.consume_weapon_resources(retal_strength);

                  const std::string short_msg =
                      GB::presentation::render_bombard_short(*result_opt);
                  const std::string long_msg =
                      GB::presentation::render_bombard_long(*result_opt);
                  post(g.entity_manager, short_msg, NewsType::COMBAT);
                  notify_star(g.session_registry, g.entity_manager, Playernum,
                              Governor, target_ship.storbits(), short_msg);
                  g.present(*result_opt);
                  warn_player(g.session_registry, g.entity_manager,
                              target_ship.owner(), target_ship.governor(),
                              long_msg);
                }
              }

              /* protecting ships retaliate individually if damage was inflicted
               */
              if (damage) {
                for (auto ship_handle : ShipList::on_planet(
                         g.entity_manager, g.snum(), g.pnum())) {
                  Ship& ship = *ship_handle;
                  if (ship.protect().on && (ship.protect().ship == toship) &&
                      ship.number() != toship && ship.alive() &&
                      ship.active()) {
                    auto [escort_strength, overload] = check_overload(
                        g.entity_manager, ship, 0, ship.check_retal_strength());
                    if (overload) {
                      notify_reactor_overload(g.entity_manager, *overload);
                      continue;
                    }

                    if (auto result2_opt = shoot_ship_to_planet(
                            g.entity_manager, ship, p, escort_strength,
                            sector_coords, smap, false, guntype_t::NONE)) {
                      ship.consume_weapon_resources(escort_strength);
                      const std::string short_msg2 =
                          GB::presentation::render_bombard_short(*result2_opt);
                      const std::string long_msg2 =
                          GB::presentation::render_bombard_long(*result2_opt);
                      post(g.entity_manager, short_msg2, NewsType::COMBAT);
                      notify_star(g.session_registry, g.entity_manager,
                                  Playernum, Governor, ship.storbits(),
                                  short_msg2);
                      g.present(*result2_opt);
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

  return fired;
}

const CommandDescriptor defend_cmd{
    .name = "defend",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 3,
    .syntax = "defend <ship> <sector> [<strength>]",
    .description =
        "Defend planet against orbiting ships using planetary defense guns",
    .handler = &defend,
};

}  // namespace GB::commands
