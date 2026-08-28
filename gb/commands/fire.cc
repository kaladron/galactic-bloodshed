// SPDX-License-Identifier: Apache-2.0

/// \file fire.cc
/// \brief Fire weapons at target ship.

module;

import gblib;
import notification;
import session;
import std;

module commands;

namespace GB::commands {

/*! Ship vs ship */
bool fire(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  int cew_mode;
  // This is called from dock.cc.
  if (argv[0] == "fire-from-dock") {
    // TODO(jeffbailey): It's not clear that cew is ever used as anything other
    // than a true/false value.
    cew_mode = 3;
  } else if (argv[0] == "cew") {
    cew_mode = 1;
  } else {  // argv[0] = fire
    cew_mode = 0;
  }
  shipnum_t toship;
  shipnum_t sh;
  int strength;
  int maxstrength;
  int retal;
  bool any_fired = false;

  sh = 0;  // TODO(jeffbailey): No idea what this is, init to 0.

  if (argv.size() < 3) {
    std::string msg =
        "Syntax: '" + argv[0] + " <ship> <target> [<strength>]'.\n";
    g.out << msg;
    return false;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& from = *ship_handle;

    if (!ship_matches_filter(argv[1], from)) continue;
    if (!authorized(Governor, from)) continue;
    if (!from.active()) {
      g.out << std::format("{} is irradiated and inactive.\n", from);
      continue;
    }
    if (argv[0] != "fire-from-dock") {
      if (from.whatorbits() == ScopeLevel::LEVEL_UNIV) {
        if (!g.deduct_univ_ap(1)) {
          g.out << "You need 1 universe action points.\n";
          continue;
        }
      } else {
        if (!g.deduct_ap(from.storbits(), 1)) {
          g.out << "You don't have 1 action points there.\n";
          continue;
        }
      }
    }
    if (cew_mode) {
      if (!from.cew()) {
        g.out << "That ship is not equipped to fire CEWs.\n";
        continue;
      }
      if (!from.mounted()) {
        g.out << "You need to have a crystal mounted to fire CEWs.\n";
        continue;
      }
    }
    auto toshiptmp = string_to_shipnum(argv[2]);
    if (!toshiptmp || *toshiptmp <= 0) {
      g.out << "Bad ship number.\n";
      return any_fired;
    }
    toship = *toshiptmp;
    if (toship == from.number()) {
      g.out << "Get real.\n";
      continue;
    }
    const Ship* to;
    try {
      to = g.entity_manager.peek_ship(toship);
    } catch (const EntityNotFoundError&) {
      continue;
    }

    /* save defense attack strength for retaliation */
    // Calculate retaliation strength BEFORE damage is applied.
    // This pre-damage strength will be used if the target retaliates,
    // even though the ship itself will be modified by taking damage.
    retal = check_retal_strength(*to);

    if (from.type() == ShipType::OTYPE_AFV) {
      if (!landed(from)) {
        g.out << std::format("{} isn't landed on a planet!\n", from);
        continue;
      }
      if (!landed(*to)) {
        g.session_registry.notify_player(
            Playernum, Governor,
            std::format("{} isn't landed on a planet!\n", *to));
        continue;
      }
    }
    if (landed(from) && landed(*to)) {
      if ((from.storbits() != to->storbits()) ||
          (from.pnumorbits() != to->pnumorbits())) {
        g.out << "Landed ships can only attack other "
                 "landed ships if they are on the same "
                 "planet!\n";
        continue;
      }
      const auto* p =
          g.entity_manager.peek_planet(from.storbits(), from.pnumorbits());
      if (!adjacent(*p, from.land_coords(), to->land_coords())) {
        g.out << "You are not adjacent to your target!\n";
        continue;
      }
    }
    if (cew_mode) {
      if (from.fuel() < (double)from.cew()) {
        g.out << std::format("You need {} fuel to fire CEWs.\n", from.cew());
        continue;
      }
      if (landed(from) || landed(*to)) {
        g.out << "CEWs cannot originate from or targeted "
                 "to ships landed on planets.\n";
        continue;
      }
      g.out << std::format("CEW strength {}.\n", from.cew());
      strength = from.cew() / 2;

    } else {
      maxstrength = check_retal_strength(from);

      if (argv.size() >= 4)
        strength = std::stoi(argv[3]);
      else
        strength = check_retal_strength(from);

      if (strength > maxstrength) {
        strength = maxstrength;
        g.out << std::format("{} set to {}\n",
                             (laser_on(from) ? "Laser strength" : "Guns"),
                             strength);
      }
    }

    /* check to see if there is crystal overloads */
    if (laser_on(from) || cew_mode)
      check_overload(g.entity_manager, from, cew_mode, &strength);

    if (strength <= 0) {
      g.out << "No attack.\n";
      continue;
    }

    // Target ship attack and retaliation
    int damage = 0;
    g.entity_manager.mutate_ship(toship, [&](Ship& to_ship) {
      auto s2sresult = shoot_ship_to_ship(g.entity_manager, from, to_ship,
                                          strength, cew_mode);

      if (!s2sresult) {
        g.out << "Illegal attack.\n";
        return;
      }

      auto const& [dmg, short_buf, long_buf] = *s2sresult;
      damage = dmg;
      any_fired = true;

      if (laser_on(from) || cew_mode)
        use_fuel(from, 2.0 * (double)strength);
      else
        use_destruct(from, strength);

      if (!to_ship.alive()) post(g.entity_manager, short_buf, NewsType::COMBAT);
      notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                  from.storbits(), short_buf);
      warn_player(g.session_registry, g.entity_manager, to_ship.owner(),
                  to_ship.governor(), long_buf);
      g.out << long_buf;
      /* defending ship retaliates */

      strength = 0;
      if (retal && damage && to_ship.protect().self) {
        // Use pre-damage retaliation strength (saved in 'retal' above).
        // shoot_ship_to_ship() uses the explicit strength parameter,
        // not the ship's current damage state, so this correctly applies
        // the ship's original (pre-damage) attack capability.
        strength = retal;
        if (laser_on(to_ship))
          check_overload(g.entity_manager, to_ship, 0, &strength);

        auto retal_result = shoot_ship_to_ship(g.entity_manager, to_ship, from,
                                               strength, 0, true);
        if (retal_result) {
          auto const& [r_damage, r_short_buf, r_long_buf] = *retal_result;

          if (laser_on(to_ship))
            use_fuel(to_ship, 2.0 * (double)strength);
          else
            use_destruct(to_ship, strength);
          if (!from.alive())
            post(g.entity_manager, r_short_buf, NewsType::COMBAT);
          notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                      from.storbits(), r_short_buf);
          g.out << r_long_buf;
          warn_player(g.session_registry, g.entity_manager, to_ship.owner(),
                      to_ship.governor(), r_long_buf);
        }
      }
    });

    if (!damage) {
      continue;
    }

    /* protecting ships retaliate individually if damage was inflicted */
    /* AFVs immune to retaliation of this type */
    if (from.alive() && from.type() != ShipType::OTYPE_AFV) {
      if (to->whatorbits() == ScopeLevel::LEVEL_STAR) { /* star level ships */
        g.entity_manager.with_star(
            to->storbits(), [&](const Star& star) { sh = star.ships(); });
      }
      if (to->whatorbits() == ScopeLevel::LEVEL_PLAN) { /* planet level ships */
        g.entity_manager.with_planet(to->storbits(), to->pnumorbits(),
                                     [&](const Planet& p) { sh = p.ships(); });
      }
      ShipList shiplist(g.entity_manager, sh);
      for (auto ship_handle : shiplist) {
        if (!from.alive()) break;
        Ship& ship = *ship_handle;
        if (ship.protect().on && (ship.protect().ship == toship) &&
            (ship.protect().ship == toship) && ship.number() != from.number() &&
            ship.number() != toship && ship.alive() && ship.active()) {
          strength = check_retal_strength(ship);
          if (laser_on(ship))
            check_overload(g.entity_manager, ship, 0, &strength);

          auto s2sresult =
              shoot_ship_to_ship(g.entity_manager, ship, from, strength, 0);
          if (s2sresult) {
            auto const& [damange, short_buf, long_buf] = *s2sresult;
            if (laser_on(ship))
              use_fuel(ship, 2.0 * (double)strength);
            else
              use_destruct(ship, strength);
            if (!from.alive())
              post(g.entity_manager, short_buf, NewsType::COMBAT);
            notify_star(g.session_registry, g.entity_manager, Playernum,
                        Governor, from.storbits(), short_buf);
            g.out << long_buf;
            warn_player(g.session_registry, g.entity_manager, ship.owner(),
                        ship.governor(), long_buf);
          }
        }
      }
    }

    any_fired = true;
  }  // end of ShipList iteration

  return any_fired;
}

bool cew(const command_t& argv, GameObj& g) {
  return fire(argv, g);
}

const CommandDescriptor fire_cmd{
    .name = "fire",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "fire <ship> <target> [<strength>]",
    .description = "Fire conventional or laser weapons at target ship",
    .handler = &fire,
};

const CommandDescriptor cew_cmd{
    .name = "cew",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "cew <ship> <target>",
    .description = "Fire Confined Energy Weapons (CEWs) at target ship",
    .handler = &cew,
};

}  // namespace GB::commands
