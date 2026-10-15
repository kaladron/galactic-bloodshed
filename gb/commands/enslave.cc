// SPDX-License-Identifier: Apache-2.0

/// \file enslave.cc
/// \brief Enslave planet population.

module commands;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

namespace GB::commands {

bool enslave(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  constexpr ap_t AP_COUNT = 2;

  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno) {
    g.out << "Specify a valid ship number.\n";
    return false;
  }

  const Ship* s_ptr = nullptr;
  try {
    s_ptr = g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return false;
  }
  const Ship& s = *s_ptr;
  if (!g.check_commandable(s)) {
    return false;
  }
  if (s.type() != ShipType::STYPE_OAP) {
    g.out << std::format("This ship is not an {}.\n",
                         ship_template(ShipType::STYPE_OAP).name);
    return false;
  }
  if (s.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    g.out << std::format("{} doesn't orbit a planet.\n", s);
    return false;
  }

  const auto& p_ref =
      *g.entity_manager.peek_planet(s.storbits(), s.pnumorbits());
  if (p_ref.sectors_owned_by(Playernum) == 0) {
    g.out << "You don't have a garrison on the planet.\n";
    return false;
  }

  bool has_aliens = false;
  resource_t defense_strength = 0;
  resource_t attack_strength = 0;
  for (const auto& [i, info] : p_ref.info_map()) {
    if (info.numsectsowned && i != Playernum) {
      has_aliens = true;
      defense_strength += info.destruct;
    }
  }

  if (!has_aliens) {
    g.out << "There is no one else on this planet to enslave!\n";
    return false;
  }

  if (!g.deduct_ap(s.storbits(), AP_COUNT)) {
    g.out << "You don't have enough action points.\n";
    return false;
  }

  for (const Ship& s2 : ShipList::readonly_on_planet(
           g.entity_manager, s.storbits(), s.pnumorbits())) {
    if (s2.alive() && s2.active()) {
      if (p_ref.sectors_owned_by(s2.owner()) && s2.owner() != Playernum) {
        defense_strength += s2.destruct();
      } else if (s2.owner() == Playernum) {
        attack_strength += s2.destruct();
      }
    }
  }

  const bool enslaved = defense_strength <= 2 * attack_strength;
  if (enslaved) {
    g.entity_manager.mutate_planet(s.storbits(), s.pnumorbits(),
                                   [&](Planet& p) { p.enslave_to(Playernum); });
  }

  const auto& star = *g.entity_manager.peek_star(s.storbits());
  GB::presentation::EnslaveResult result{
      .enslaved = enslaved,
      .orbit_path = prin_ship_orbits(g.entity_manager, s),
      .enslaver_race_name = g.race->name,
      .attack_strength = attack_strength,
      .defense_strength = defense_strength,
      .required_garrison = p_ref.popn() * 0.001,
  };
  g.present(result);

  const std::string telegram = GB::presentation::render_enslave_telegram(
      star.get_name(), star.get_planet_name(s.pnumorbits()),
      std::format("{}", s), result);
  for (const auto& [i, info] : p_ref.info_map()) {
    if (info.numsectsowned && i != Playernum) {
      warn_player(g.session_registry, g.entity_manager, i, star.governor(i),
                  telegram);
    }
  }

  return true;
}

const CommandDescriptor enslave_cmd{
    .name = "enslave",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "enslave <ship>",
    .description =
        "Enslave enemy planet population using an Orbiting Assault Platform",
    .handler = &enslave,
};

}  // namespace GB::commands
