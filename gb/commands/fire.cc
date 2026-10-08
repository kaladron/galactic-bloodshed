// SPDX-License-Identifier: Apache-2.0

/// \file fire.cc
/// \brief Fire weapons at target ship.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Validate surface-to-surface and AFV combat geometry constraints.
 */
bool validate_surface_combat_geometry(GameObj& g, const Ship& from,
                                      const Ship& to) {
  if (from.type() == ShipType::OTYPE_AFV) {
    if (!from.is_landed()) {
      g.out << std::format("{} isn't landed on a planet!\n", from);
      return false;
    }
    if (!to.is_landed()) {
      g.out << std::format("{} isn't landed on a planet!\n", to);
      return false;
    }
  }

  if (from.is_landed() && to.is_landed()) {
    if (from.storbits() != to.storbits() ||
        from.pnumorbits() != to.pnumorbits()) {
      g.out << "Landed ships can only attack other landed ships if they are on "
               "the same planet!\n";
      return false;
    }
    const auto* p =
        g.entity_manager.peek_planet(from.storbits(), from.pnumorbits());
    if (!p->is_adjacent(from.land_coords(), to.land_coords())) {
      g.out << "You are not adjacent to your target!\n";
      return false;
    }
  }
  return true;
}

enum class FireMode {
  Normal,
  Cew,
  DefensiveFromDock,
};

/**
 * @brief Validate weapon equipment, fuel, and requested attack strength.
 */
std::optional<weapon_power_t>
compute_fire_strength(const command_t& argv, GameObj& g, const Ship& from,
                      const Ship& to, FireMode mode) {
  if (mode == FireMode::Cew) {
    if (!from.cew()) {
      g.out << "That ship is not equipped to fire CEWs.\n";
      return std::nullopt;
    }
    if (!from.mounted()) {
      g.out << "You need to have a crystal mounted to fire CEWs.\n";
      return std::nullopt;
    }
    if (from.fuel() < static_cast<double>(from.cew())) {
      g.out << std::format("You need {} fuel to fire CEWs.\n", from.cew());
      return std::nullopt;
    }
    if (from.is_landed() || to.is_landed()) {
      g.out << "CEWs cannot originate from or targeted to ships landed on "
               "planets.\n";
      return std::nullopt;
    }
    g.out << std::format("CEW strength {}.\n", from.cew());
    return static_cast<weapon_power_t>(from.cew() / 2);
  }

  const auto maxstrength = from.check_retal_strength();
  auto strength = maxstrength;
  if (argv.size() >= 4) {
    auto parsed = scn::scan<weapon_power_t>(argv[3], "{}");
    if (!parsed) {
      g.out << "No attack.\n";
      return std::nullopt;
    }
    strength = parsed->value();
  }

  if (strength > maxstrength) {
    strength = maxstrength;
    g.out << std::format("{} set to {}\n",
                         from.is_laser_on() ? "Laser strength" : "Guns",
                         strength);
  }
  return strength;
}

/**
 * @brief Verify that the player has sufficient Universe or Star AP to fire
 * from a ship (unless invoked internally via `fire-from-dock`).
 */
bool has_fire_ap(FireMode mode, GameObj& g, const Ship& from) {
  if (mode == FireMode::DefensiveFromDock) {
    return true;
  }
  if (from.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (g.entity_manager.peek_universe()->get_AP(g.player()) < 1) {
      g.out << "You need 1 universe action points.\n";
      return false;
    }
    return true;
  }
  if (g.entity_manager.peek_star(from.storbits())->AP(g.player()) < 1) {
    g.out << "You don't have 1 action points there.\n";
    return false;
  }
  return true;
}

/**
 * @brief Deduct 1 Universe or Star AP after firing from a ship (unless invoked
 * internally via `fire-from-dock`).
 */
void deduct_fire_ap(FireMode mode, GameObj& g, const Ship& from) {
  if (mode == FireMode::DefensiveFromDock) {
    return;
  }
  if (from.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    g.deduct_univ_ap(1);
    return;
  }
  g.deduct_ap(from.storbits(), 1);
}

/**
 * @brief Execute self-defense retaliation from the attacked target ship using
 * its pre-damage attack capability.
 */
void resolve_target_self_retaliation(GameObj& g, Ship& from, Ship& to_ship,
                                     weapon_power_t retal, damage_t damage) {
  if (!retal || !damage || !to_ship.protect().retaliate) {
    return;
  }

  auto [strength, overload] =
      check_overload(g.entity_manager, to_ship, 0, retal);
  if (overload) {
    GB::commands::notify_reactor_overload(g.entity_manager, *overload);
    return;
  }

  auto retal_result =
      shoot_ship_to_ship(g.entity_manager, to_ship, from, strength, 0, true);
  if (!retal_result) {
    return;
  }

  const std::string r_short_buf =
      GB::presentation::render_ship_shot_short(*retal_result);
  const std::string r_long_buf =
      GB::presentation::render_ship_shot_long(*retal_result);
  to_ship.consume_weapon_resources(strength);
  if (!from.alive()) {
    post(g.entity_manager, r_short_buf, NewsType::COMBAT);
  }
  notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
              from.storbits(), r_short_buf);
  g.present(*retal_result);
  warn_player(g.session_registry, g.entity_manager, to_ship.owner(),
              to_ship.governor(), r_long_buf);
}

/**
 * @brief Execute escort retaliation (`protect().ship == toship`) when damage
 * was inflicted on the protected ship.
 */
void resolve_escort_retaliation(GameObj& g, Ship& from, const Ship& to,
                                shipnum_t toship) {
  if (!from.alive() || from.type() == ShipType::OTYPE_AFV) {
    return;
  }

  ShipList shiplist = (to.whatorbits() == ScopeLevel::LEVEL_STAR)
                          ? ShipList::in_star(g.entity_manager, to.storbits())
                          : ShipList::on_planet(g.entity_manager, to.storbits(),
                                                to.pnumorbits());
  for (auto ship_handle : shiplist) {
    if (!from.alive()) break;
    Ship& ship = *ship_handle;
    if (!ship.protect().on || ship.protect().ship != toship ||
        ship.number() == from.number() || ship.number() == toship ||
        !ship.alive() || !ship.active()) {
      continue;
    }

    auto [strength, overload] =
        check_overload(g.entity_manager, ship, 0, ship.check_retal_strength());
    if (overload) {
      GB::commands::notify_reactor_overload(g.entity_manager, *overload);
      continue;
    }

    if (auto s2sresult =
            shoot_ship_to_ship(g.entity_manager, ship, from, strength, 0)) {
      const std::string short_buf =
          GB::presentation::render_ship_shot_short(*s2sresult);
      const std::string long_buf =
          GB::presentation::render_ship_shot_long(*s2sresult);
      ship.consume_weapon_resources(strength);
      if (!from.alive()) {
        post(g.entity_manager, short_buf, NewsType::COMBAT);
      }
      notify_star(g.session_registry, g.entity_manager, g.player(),
                  g.governor(), from.storbits(), short_buf);
      g.present(*s2sresult);
      warn_player(g.session_registry, g.entity_manager, ship.owner(),
                  ship.governor(), long_buf);
    }
  }
}

/**
 * @brief Execute ship-to-ship fire from a single attacking ship.
 */
bool fire_from_ship(const command_t& argv, GameObj& g, Ship& from,
                    shipnum_t toship, FireMode mode) {
  if (toship == from.number()) {
    g.out << "Get real.\n";
    return false;
  }

  const Ship* to = nullptr;
  try {
    to = g.entity_manager.peek_ship(toship);
  } catch (const EntityNotFoundError&) {
    return false;
  }

  if (!validate_surface_combat_geometry(g, from, *to)) {
    return false;
  }

  auto strength_opt = compute_fire_strength(argv, g, from, *to, mode);
  if (!strength_opt) {
    return false;
  }
  const auto initial_strength = *strength_opt;

  if (!has_fire_ap(mode, g, from)) {
    return false;
  }

  if (initial_strength <= 0) {
    g.out << "No attack.\n";
    return false;
  }

  const bool is_cew = (mode == FireMode::Cew);
  const int cew_range_flag = is_cew ? 1 : 0;

  auto [strength, overload] =
      check_overload(g.entity_manager, from, cew_range_flag, initial_strength);
  if (overload) {
    GB::commands::notify_reactor_overload(g.entity_manager, *overload);
    g.out << "No attack.\n";
    deduct_fire_ap(mode, g, from);
    return true;
  }

  const auto retal = to->check_retal_strength();
  damage_t damage = 0;
  bool fired = false;

  g.entity_manager.mutate_ship(toship, [&](Ship& to_ship) {
    auto s2sresult = shoot_ship_to_ship(g.entity_manager, from, to_ship,
                                        strength, cew_range_flag);
    if (!s2sresult) {
      g.out << "Illegal attack.\n";
      return;
    }

    damage = s2sresult->damage;
    fired = true;

    const std::string short_buf =
        GB::presentation::render_ship_shot_short(*s2sresult);
    const std::string long_buf =
        GB::presentation::render_ship_shot_long(*s2sresult);

    from.consume_weapon_resources(strength, is_cew);

    if (!to_ship.alive()) {
      post(g.entity_manager, short_buf, NewsType::COMBAT);
    }
    notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
                from.storbits(), short_buf);
    warn_player(g.session_registry, g.entity_manager, to_ship.owner(),
                to_ship.governor(), long_buf);
    g.present(*s2sresult);

    resolve_target_self_retaliation(g, from, to_ship, retal, damage);
  });

  if (fired) {
    deduct_fire_ap(mode, g, from);
  }

  if (damage > 0) {
    resolve_escort_retaliation(g, from, *to, toship);
  }
  return fired;
}

}  // namespace

namespace GB::commands {

void notify_reactor_overload(EntityManager& em,
                             const ReactorOverloadEvent& event) {
  const std::string message =
      GB::presentation::render_reactor_overload_event(event);
  push_telegram(em, event.owner, event.governor, message);
  if (event.outcome == ReactorOverloadOutcome::ShipExploded) {
    post(em, message, NewsType::COMBAT);
    if (event.scope != ScopeLevel::LEVEL_UNIV) {
      telegram_star(em, event.star_id, event.owner, event.governor, message);
    }
  }
}

/*! Ship vs ship */
bool fire(const command_t& argv, GameObj& g) {
  FireMode mode = FireMode::Normal;
  if (argv[0] == "fire-from-dock") {
    mode = FireMode::DefensiveFromDock;
  } else if (argv[0] == "cew") {
    mode = FireMode::Cew;
  }

  if (argv.size() < 3) {
    g.out << std::format("Syntax: '{} <ship> <target> [<strength>]'.\n",
                         argv[0]);
    return false;
  }

  auto toshiptmp = string_to_shipnum(argv[2]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }
  const shipnum_t toship = *toshiptmp;
  bool any_fired = false;

  if (mode == FireMode::DefensiveFromDock) {
    const auto from_no = parse_ship_selection(argv[1]);
    if (!from_no || *from_no <= 0) {
      return false;
    }
    try {
      g.entity_manager.mutate_ship(*from_no, [&](Ship& from) {
        if (from.alive() && from.active()) {
          any_fired = fire_from_ship(argv, g, from, toship, mode);
        }
      });
    } catch (const EntityNotFoundError&) {
      return false;
    }
    return any_fired;
  }

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    Ship& from = *ship_handle;

    if (!from.active()) {
      g.out << std::format("{} is irradiated and inactive.\n", from);
      continue;
    }

    if (fire_from_ship(argv, g, from, toship, mode)) {
      any_fired = true;
    }
  }

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
