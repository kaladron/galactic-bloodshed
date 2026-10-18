// SPDX-License-Identifier: Apache-2.0

/// \file load.cc
/// \brief Functions for loading and unloading commodities to/from ships.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

void dispatch_transporter(const Race& race, GameObj& g, TransporterShip& s) {
  const auto res = execute_transporter_transfer(
      g.entity_manager, s, g.snum(), g.pnum(), race.mass, race.Metamorph);
  if (!res) {
    g.out << GB::presentation::format_transporter_error(res.error());
    return;
  }
  g.present(*res);
  if (auto tele = GB::presentation::render_transporter_telegram(*res)) {
    warn_player(g.session_registry, g.entity_manager, res->receiver_owner,
                res->receiver_governor, *tele);
  }
}

void dispatch_ship_to_ship_transfer(Ship& s, ShipCargoType cargo,
                                    std::int64_t amt, const Race& race,
                                    GameObj& g) {
  const auto result = execute_ship_to_ship_cargo_transfer(
      g.entity_manager, s, cargo, amt, race.mass, race.Metamorph);
  if (auto tele =
          GB::presentation::render_ship_cargo_transfer_telegram(result)) {
    warn_player(g.session_registry, g.entity_manager, result.target_owner,
                result.target_governor, *tele);
  }
  g.present(result);
}

bool dispatch_planet_transfer(Ship& s, ShipCargoType cargo, std::int64_t amt,
                              const Race& race, GameObj& g) {
  const auto result = execute_planet_cargo_transfer(
      g.entity_manager, s, g.player(), g.governor(), g.snum(), g.pnum(), cargo,
      amt, race.mass);
  g.present(result);

  if (result.assault.has_value()) {
    const auto& assault = *result.assault;
    if (!assault.requires_unload) {
      if (auto absorb_tele =
              GB::presentation::render_alien_assault_absorption_telegram(
                  assault)) {
        warn_player(g.session_registry, g.entity_manager,
                    assault.defender_player, assault.defender_governor,
                    *absorb_tele);
      }
      warn_player(
          g.session_registry, g.entity_manager, assault.defender_player,
          assault.defender_governor,
          GB::presentation::render_alien_assault_defender_telegram(assault));
      const auto news = GB::presentation::render_alien_assault_news(assault);
      post(g.entity_manager, news, NewsType::COMBAT);
      notify_star(g.session_registry, g.entity_manager, assault.attacker_player,
                  assault.attacker_governor, g.snum(), news);
    }
    return true;
  }
  return false;
}

}  // namespace

namespace GB::commands {

bool dispatch_cargo_transfer(Ship& s, char commod, std::int64_t amt,
                             bool default_to_lower_limit, GameObj& g,
                             bool& should_return_early) {
  const player_t Playernum = g.player();

  const auto dock_ctx = validate_ship_docking(g.entity_manager, s, Playernum,
                                              g.level(), g.snum(), g.pnum());
  if (!dock_ctx) {
    g.out << GB::presentation::format_ship_docking_error(dock_ctx.error());
    return false;
  }
  g.present(*dock_ctx);

  if (amt < 0 && s.type() == ShipType::OTYPE_VN) {
    g.out << "You can't unload VNs.\n";
    return false;
  }

  const auto cargo_opt = char_to_ship_cargo(commod);
  if (!cargo_opt) {
    g.out << "No such commodity valid.\n";
    return false;
  }
  const ShipCargoType cargo = *cargo_opt;

  CargoTransferLimits lim{};
  if (dock_ctx->is_docked_to_ship) {
    g.entity_manager.with_ship(*s.destshipno(), [&](const Ship& s2) {
      lim = compute_ship_transfer_limits(s, s2, cargo,
                                         dock_ctx->is_different_owner);
    });
  } else {
    lim = compute_planet_transfer_limits(g.entity_manager, s, Playernum, cargo,
                                         g.snum(), g.pnum());
  }

  if (amt < lim.lolim || amt > lim.uplim) {
    g.out << std::format("you can only transfer between {} and {}.\n",
                         lim.lolim, lim.uplim);
    return false;
  }

  if (amt == 0) {
    amt = default_to_lower_limit ? lim.lolim : lim.uplim;
  }

  const auto& race = *g.race;
  if (dock_ctx->is_docked_to_ship) {
    dispatch_ship_to_ship_transfer(s, cargo, amt, race, g);
  } else if (dispatch_planet_transfer(s, cargo, amt, race, g)) {
    should_return_early = true;
    return true;
  }

  if (auto* trans = s.as<TransporterShip>()) {
    if (trans->on()) {
      dispatch_transporter(race, g, *trans);
    }
  }
  return true;
}

bool load(const command_t& argv, GameObj& g) {
  if (argv[2].empty()) {
    g.out << "Load what?\n";
    return false;
  }

  std::int64_t requested_amt = 0;
  if (argv.size() > 3) {
    auto parsed_amt = scn::scan<std::int64_t>(argv[3], "{}");
    if (!parsed_amt) {
      g.out << "Invalid amount.\n";
      return false;
    }
    requested_amt = parsed_amt->value();
  }

  const char commod = argv[2][0];
  bool success = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    bool should_return_early = false;
    if (dispatch_cargo_transfer(*ship_handle, commod, requested_amt, false, g,
                                should_return_early)) {
      success = true;
      if (should_return_early) {
        return true;
      }
    }
  }
  return success;
}

const CommandDescriptor load_cmd{
    .name = "load",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "load <ship> <commodity> [<amount>]",
    .description = "Load commodities onto a ship",
    .handler = &load,
};

}  // namespace GB::commands
