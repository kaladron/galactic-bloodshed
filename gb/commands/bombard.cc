// SPDX-License-Identifier: Apache-2.0

/// \file bombard.cc
/// \brief Ship vs planet bombardment command.

module;

import session;
import gb.entities;
import gb.services;
import notification;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Validate planetary defense networks and resolve target sector
 * coordinates for bombardment.
 */
std::optional<Coordinates> resolve_bombard_target_coords(const command_t& argv,
                                                         GameObj& g,
                                                         const Ship& from,
                                                         const Planet& p) {
  const bool has_defense = has_planet_defense(g.entity_manager, p.star_id(),
                                              p.planet_order(), g.player());

  if (has_defense && !from.is_landed()) {
    g.out << "Target has planetary defense networks.\n";
    g.out << "These have to be eliminated before you can attack sectors.\n";
    return std::nullopt;
  }

  Coordinates target_coords{};
  if (argv.size() > 2) {
    auto coords_opt = Coordinates::parse(argv[2]);
    if (!coords_opt) {
      g.out << "Invalid sector format.\n";
      return std::nullopt;
    }
    target_coords = *coords_opt;
    if (!p.is_valid(target_coords)) {
      g.out << "Illegal sector.\n";
      return std::nullopt;
    }
  } else {
    g.entity_manager.with_sectormap(
        from.storbits(), from.pnumorbits(), [&](const SectorMap& smap) {
          target_coords = smap.get_random().coords();
        });
  }

  if (from.is_landed() && !p.is_adjacent(from.land_coords(), target_coords)) {
    g.out << "You are not adjacent to that sector.\n";
    return std::nullopt;
  }
  return target_coords;
}

/**
 * @brief Resolve planetary surface-to-orbit gun retaliation against a
 * bombarding ship.
 */
void resolve_planetary_retaliation(GameObj& g, Ship& from, Planet& p,
                                   const BombardResult& result) {
  if (!DEFENSE || !result.sectors_destroyed ||
      from.type() == ShipType::OTYPE_AFV || p.is_enslaved()) {
    return;
  }

  const player_t playernum = g.player();
  const governor_t governor = g.governor();
  const auto* star = g.entity_manager.peek_star(from.storbits());

  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const player_t i = race.Playernum;
    if (!result.nuked_players[i]) continue;

    g.entity_manager.mutate_race(i, [&](Race& alien) {
      const auto retal_strength =
          std::min(static_cast<weapon_power_t>(p.info(i).destruct),
                   static_cast<weapon_power_t>(p.info(i).guns));
      p.info(i).destruct -= retal_strength;

      if (auto p2s_opt = shoot_planet_to_ship(g.entity_manager, alien, from,
                                              retal_strength)) {
        const auto& [p_damage, p_short, p_long] = *p2s_opt;
        warn_player(g.session_registry, g.entity_manager, i, star->governor(i),
                    p_long);
        g.out << p_long;
        if (!from.alive()) {
          post(g.entity_manager, p_short, NewsType::COMBAT);
        }
        notify_star(g.session_registry, g.entity_manager, playernum, governor,
                    from.storbits(), p_short);
      }
    });
  }
}

/**
 * @brief Resolve orbital ship retaliation (`protect().planet`) against a
 * bombarding ship.
 */
void resolve_protector_ship_retaliation(GameObj& g, Ship& from,
                                        const BombardResult& result) {
  if (!result.sectors_destroyed || !from.alive() ||
      from.type() == ShipType::OTYPE_AFV) {
    return;
  }

  const player_t playernum = g.player();
  const governor_t governor = g.governor();

  for (auto ship_handle : ShipList::on_planet(g.entity_manager, from.storbits(),
                                              from.pnumorbits())) {
    if (!from.alive()) break;
    Ship& ship = *ship_handle;
    if (!ship.protect().planet || ship.number() == from.number() ||
        !ship.alive() || !ship.active()) {
      continue;
    }

    auto retal_strength = ship.check_retal_strength();
    if (ship.is_laser_on()) {
      check_overload(g.entity_manager, ship, 0, &retal_strength);
    }

    if (auto s2s_opt = shoot_ship_to_ship(g.entity_manager, ship, from,
                                          retal_strength, 0)) {
      const auto& [dmg, short_buf, long_buf] = *s2s_opt;
      if (ship.is_laser_on()) {
        ship.consume_fuel(ENERGY_WEAPON_FUEL_PER_STRENGTH *
                          static_cast<double>(retal_strength));
      } else {
        ship.consume_destruct(retal_strength);
      }
      if (!from.alive()) {
        post(g.entity_manager, short_buf, NewsType::COMBAT);
      }
      notify_star(g.session_registry, g.entity_manager, playernum, governor,
                  from.storbits(), short_buf);
      warn_player(g.session_registry, g.entity_manager, ship.owner(),
                  ship.governor(), long_buf);
      g.out << long_buf;
    }
  }
}

/**
 * @brief Execute planetary bombardment from a single ship.
 */
bool bombard_from_ship(const command_t& argv, GameObj& g, Ship& from) {
  if (from.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must be in orbit around a planet to bombard.\n";
    return false;
  }
  if (from.type() == ShipType::OTYPE_AFV && !from.is_landed()) {
    g.out << "This ship is not landed on the planet.\n";
    return false;
  }

  const auto maxstrength = from.check_retal_strength();
  auto strength = maxstrength;
  if (argv.size() > 3) {
    auto parsed = scn::scan<weapon_power_t>(argv[3], "{}");
    if (!parsed) {
      g.out << "No attack.\n";
      return false;
    }
    strength = parsed->value();
  }

  if (strength > maxstrength) {
    strength = maxstrength;
    g.out << std::format("{} set to {}\n",
                         from.is_laser_on() ? "Laser strength" : "Guns",
                         strength);
  }

  bool fired = false;
  g.entity_manager.mutate_planet(
      from.storbits(), from.pnumorbits(), [&](Planet& p) {
        auto coords_opt = resolve_bombard_target_coords(argv, g, from, p);
        if (!coords_opt) {
          return;
        }
        const Coordinates target_coords = *coords_opt;

        if (!g.deduct_ap(from.storbits(), 1)) {
          g.out << "You don't have 1 action points there.\n";
          return;
        }

        if (from.is_laser_on()) {
          check_overload(g.entity_manager, from, 0, &strength);
        }
        if (strength <= 0) {
          g.out << "No attack.\n";
          return;
        }

        std::optional<BombardResult> opt_result;
        g.entity_manager.mutate_sectormap(
            from.storbits(), from.pnumorbits(), [&](SectorMap& smap) {
              opt_result = shoot_ship_to_planet(g.entity_manager, from, p,
                                                strength, target_coords, smap,
                                                false, guntype_t::NONE);
            });

        if (!opt_result) {
          g.out << "Illegal attack.\n";
          return;
        }
        const auto& result = *opt_result;

        if (from.is_laser_on()) {
          from.consume_fuel(ENERGY_WEAPON_FUEL_PER_STRENGTH *
                            static_cast<double>(strength));
        } else {
          from.consume_destruct(strength);
        }

        post(g.entity_manager, result.short_message, NewsType::COMBAT);
        notify_star(g.session_registry, g.entity_manager, g.player(),
                    g.governor(), from.storbits(), result.short_message);
        const auto* star = g.entity_manager.peek_star(from.storbits());
        for (const Race& race : RaceList::readonly(g.entity_manager)) {
          const player_t i = race.Playernum;
          if (result.nuked_players[i]) {
            warn_player(g.session_registry, g.entity_manager, i,
                        star->governor(i), result.long_message);
          }
        }
        g.out << result.long_message;

        resolve_planetary_retaliation(g, from, p, result);
        resolve_protector_ship_retaliation(g, from, result);
        fired = true;
      });

  return fired;
}

}  // namespace

namespace GB::commands {

/*! Ship vs planet */
bool bombard(const command_t& argv, GameObj& g) {
  const governor_t governor = g.governor();
  bool any_fired = false;

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& from = *ship_handle;

    if (!ship_matches_filter(argv[1], from)) continue;
    if (!from.is_authorized_for(governor)) continue;
    if (!from.active()) {
      g.out << std::format("{} is irradiated and inactive.\n", from);
      continue;
    }

    if (bombard_from_ship(argv, g, from)) {
      any_fired = true;
    }
  }

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
