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

void transfer_single_transporter_cargo(TransporterShip& s, Ship& s2,
                                       ShipCargoType type, std::int64_t amount,
                                       double mass, std::string_view label,
                                       std::string_view tele_label, GameObj& g,
                                       std::string& tele_lines) {
  if (amount <= 0) return;
  auto transferred = s.transfer_cargo_to(s2, type, amount, mass);
  if (transferred > 0) {
    g.out << std::format("{} {} transferred.\n", transferred, label);
    tele_lines += std::format("{} {}\n", transferred, tele_label);
  }
}

std::string transfer_transporter_cargo(TransporterShip& s, Ship& s2,
                                       const Race& race, GameObj& g) {
  std::string tele_lines;
  transfer_single_transporter_cargo(s, s2, ShipCargoType::Resource,
                                    s.resource(), 0.0, "resources", "Resources",
                                    g, tele_lines);
  transfer_single_transporter_cargo(s, s2, ShipCargoType::Fuel, s.fuel_units(),
                                    0.0, "fuel", "Fuel", g, tele_lines);
  transfer_single_transporter_cargo(s, s2, ShipCargoType::Destruct,
                                    s.destruct(), 0.0, "destruct", "Destruct",
                                    g, tele_lines);
  transfer_single_transporter_cargo(
      s, s2, ShipCargoType::Crew, s.popn(), race.mass, "population",
      race.Metamorph ? "tons of biomass" : "population", g, tele_lines);
  transfer_single_transporter_cargo(s, s2, ShipCargoType::Troops, s.troops(),
                                    race.mass, "troops", "troops", g,
                                    tele_lines);
  transfer_single_transporter_cargo(s, s2, ShipCargoType::Crystal, s.crystals(),
                                    0.0, "crystal(s)", "crystal(s)", g,
                                    tele_lines);
  return tele_lines;
}

void do_transporter(const Race& race, GameObj& g, TransporterShip& s) {
  if (!s.is_landed()) {
    g.out << "Origin ship not landed.\n";
    return;
  }
  if (s.storbits() != g.snum() || s.pnumorbits() != g.pnum()) {
    g.out << "Change scope to the planet the ship is landed on!\n";
    return;
  }
  if (s.damage()) {
    g.out << "Origin device is damaged.\n";
    return;
  }
  if (!s.target_ship()) {
    g.out << "The hopper seems to be blocked.\n";
    return;
  }

  try {
    g.entity_manager.mutate_ship(*s.target_ship(), [&](Ship& s2) {
      if (!s2.alive() || s2.type() != ShipType::OTYPE_TRANSDEV || !s2.on()) {
        g.out << "The target device is not receiving.\n";
        return;
      }
      if (!s2.is_landed()) {
        g.out << "Target ship not landed.\n";
        return;
      }
      if (s2.damage()) {
        g.out << "Target device is damaged.\n";
        return;
      }

      g.out << "Zap\07!\n";
      std::string tele_lines = transfer_transporter_cargo(s, s2, race, g);

      if (s2.owner() != s.owner()) {
        std::string telegram =
            std::format("Audio-vibatory-physio-molecular transport device #{} "
                        "gave your ship "
                        "{} the following:\n{}",
                        s, s2, tele_lines);
        warn_player(g.session_registry, g.entity_manager, s2.owner(),
                    s2.governor(), telegram);
      }
    });
  } catch (const EntityNotFoundError&) {
    g.out << "The hopper seems to be blocked.\n";
  }
}

struct DockingContext {
  bool is_docked_to_ship{false};
  bool is_different_owner{false};
};

std::optional<DockingContext> validate_ship_docking(Ship& s, GameObj& g) {
  if (!s.active()) {
    g.out << std::format("{} is irradiated and inactive.\n", s);
    return std::nullopt;
  }
  if (!s.docked()) {
    g.out << std::format("{} is not landed or docked.\n", s);
    return std::nullopt;
  }

  if (s.whatdest() == ScopeLevel::LEVEL_PLAN) {
    g.out << std::format("{} at {}\n", s, s.land_coords());
    if (g.level() != ScopeLevel::LEVEL_PLAN || s.storbits() != g.snum() ||
        s.pnumorbits() != g.pnum()) {
      g.out << "Change scope to the planet this ship is landed on.\n";
      return std::nullopt;
    }
    return DockingContext{.is_docked_to_ship = false,
                          .is_different_owner = false};
  }

  if (!s.destshipno()) {
    g.out << std::format("{} is not docked.\n", s);
    return std::nullopt;
  }

  std::optional<DockingContext> ctx;
  try {
    g.entity_manager.with_ship(*s.destshipno(), [&](const Ship& s2) {
      bool mutually_docked = (s.whatorbits() == ScopeLevel::LEVEL_SHIP ||
                              s2.destshipno() == s.number());
      if (!s2.alive() || !mutually_docked) {
        s.launch_to_orbit(s.whatorbits());
        g.out << std::format("{} is not docked.\n", s2);
        return;
      }
      if (s2.is_overloaded() && s2.whatorbits() == ScopeLevel::LEVEL_SHIP) {
        g.out << std::format("{} is overloaded!\n", s2);
        return;
      }
      g.out << std::format("{} docked with {}\n", s, s2);
      bool diff = (s2.owner() != g.player());
      if (diff) {
        g.out << std::format("Player {} owns that ship.\n", s2.owner());
      }
      ctx =
          DockingContext{.is_docked_to_ship = true, .is_different_owner = diff};
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Destination ship is bogus.\n";
    return std::nullopt;
  }
  return ctx;
}

CargoTransferLimits compute_ship_transfer_limits(const Ship& s, const Ship& s2,
                                                 ShipCargoType cargo,
                                                 bool diff) {
  CargoTransferLimits lim{};
  switch (cargo) {
    case ShipCargoType::Crystal:
      lim.uplim =
          diff ? 0
               : std::min<std::int64_t>(
                     s2.crystals(), s.max_crystals_capacity() - s.crystals());
      lim.lolim =
          diff ? 0
               : -std::min<std::int64_t>(
                     s.crystals(), s2.max_crystals_capacity() - s2.crystals());
      break;
    case ShipCargoType::Crew:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(
                             s2.popn(), s.max_crew_capacity() - s.popn());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(
                             s.popn(), s2.max_crew_capacity() - s2.popn());
      break;
    case ShipCargoType::Troops:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(s2.troops(),
                                                s.available_mil() - s.troops());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(
                             s.troops(), s2.available_mil() - s2.troops());
      break;
    case ShipCargoType::Destruct:
      lim.uplim =
          diff ? 0
               : std::min<std::int64_t>(
                     s2.destruct(), s.max_destruct_capacity() - s.destruct());
      lim.lolim = -std::min<std::int64_t>(
          s.destruct(), s2.max_destruct_capacity() - s2.destruct());
      break;
    case ShipCargoType::Fuel:
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(s2.fuel_units(),
                                                s.available_fuel_capacity());
      lim.lolim =
          -std::min<std::int64_t>(s.fuel_units(), s2.available_fuel_capacity());
      break;
    case ShipCargoType::Resource:
      if (s.can_strap_cargo_to_hull()) {
        lim.uplim = diff ? 0 : s2.resource();
      } else {
        lim.uplim =
            diff ? 0
                 : std::min<std::int64_t>(
                       s2.resource(), s.max_resource_capacity() - s.resource());
      }
      if (s2.can_strap_cargo_to_hull()) {
        lim.lolim = -s.resource();
      } else {
        lim.lolim = -std::min<std::int64_t>(
            s.resource(), s2.max_resource_capacity() - s2.resource());
      }
      break;
  }
  return lim;
}

void print_post_transfer_message(const Ship& s, ShipCargoType cargo,
                                 std::int64_t amt, GameObj& g) {
  switch (cargo) {
    case ShipCargoType::Crew:
      g.out << std::format("crew complement of {} is now {}.\n", s, s.popn());
      break;
    case ShipCargoType::Troops:
      g.out << std::format("troop complement of {} is now {}.\n", s,
                           s.troops());
      break;
    case ShipCargoType::Destruct:
      g.out << std::format("{} destruct transferred.\n", amt);
      if (!s.max_crew_capacity()) {
        g.out << std::format("\n{} {}\n", s,
                             s.destruct() ? "now boobytrapped."
                                          : "no longer boobytrapped.");
      }
      break;
    case ShipCargoType::Crystal:
      g.out << std::format("{} crystal(s) transferred.\n", amt);
      break;
    case ShipCargoType::Fuel:
      g.out << std::format("{} fuel transferred.\n", amt);
      break;
    case ShipCargoType::Resource:
      g.out << std::format("{} resources transferred.\n", amt);
      break;
  }
}

void execute_ship_to_ship_transfer(Ship& s, ShipCargoType cargo,
                                   std::int64_t amt, const Race& race,
                                   GameObj& g) {
  if (!s.destshipno()) {
    return;
  }
  g.entity_manager.mutate_ship(*s.destshipno(), [&](Ship& s2) {
    if (amt > 0) {
      s2.transfer_cargo_to(s, cargo, amt, race.mass);
    } else {
      s.transfer_cargo_to(s2, cargo, -amt, race.mass);
    }

    std::string tele_lines;
    switch (cargo) {
      case ShipCargoType::Resource:
        tele_lines += std::format("{} Resources\n", std::abs(amt));
        break;
      case ShipCargoType::Fuel:
        tele_lines += std::format("{} Fuel\n", std::abs(amt));
        break;
      case ShipCargoType::Destruct:
        tele_lines += std::format("{} Destruct\n", std::abs(amt));
        break;
      case ShipCargoType::Crystal:
        tele_lines += std::format("{} Crystal(s)\n", std::abs(amt));
        break;
      case ShipCargoType::Crew:
      case ShipCargoType::Troops:
        tele_lines +=
            std::format("{} {}\n", std::abs(amt),
                        race.Metamorph ? "tons of biomass" : "population");
        break;
    }
    if (!tele_lines.empty() && s2.owner() != s.owner() && amt < 0) {
      warn_player(
          g.session_registry, g.entity_manager, s2.owner(), s2.governor(),
          std::format(
              "Audio-vibatory-physio-molecular transport device #{} gave "
              "your ship {} the following:\n{}",
              s, s2, tele_lines));
    }
  });

  print_post_transfer_message(s, cargo, amt, g);
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

bool process_ship_load(Ship& s, std::string_view filter, char commod,
                       std::int64_t requested_amt, bool is_unload, GameObj& g,
                       bool& should_return_early) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (!GB::ship_matches_filter(filter, s) || !s.is_authorized_for(Governor) ||
      s.owner() != Playernum || !s.alive()) {
    return false;
  }

  auto dock_ctx = validate_ship_docking(s, g);
  if (!dock_ctx) {
    return false;
  }

  std::int64_t amt = is_unload ? -requested_amt : requested_amt;
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
  if (dock_ctx->is_docked_to_ship && s.destshipno()) {
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
    amt = is_unload ? lim.lolim : lim.uplim;
  }

  const auto& race = *g.race;
  if (dock_ctx->is_docked_to_ship) {
    execute_ship_to_ship_transfer(s, cargo, amt, race, g);
  } else {
    if (dispatch_planet_transfer(s, cargo, amt, race, g)) {
      should_return_early = true;
      return true;
    }
  }

  if (auto* trans = s.as<TransporterShip>()) {
    if (trans->on()) {
      do_transporter(race, g, *trans);
    }
  }
  return true;
}

}  // namespace

namespace GB::commands {

bool load(const command_t& argv, GameObj& g) {
  bool is_unload = (argv[0] == "unload");
  bool success = false;

  if (argv[2].empty()) {
    g.out << (is_unload ? "Unload what?\n" : "Load what?\n");
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

  char commod = argv[2][0];

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    bool should_return_early = false;
    if (process_ship_load(*ship_handle, argv[1], commod, requested_amt,
                          is_unload, g, should_return_early)) {
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

const CommandDescriptor unload_cmd{
    .name = "unload",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "unload <ship> <commodity> [<amount>]",
    .description = "Unload commodities from a ship",
    .handler = &load,
};

}  // namespace GB::commands
