// SPDX-License-Identifier: Apache-2.0

/// \file load.cc
/// \brief Functions for loading and unloading commodities to/from ships.

module;

import gb.entities;
import gb.services;
import notification;
import scnlib;
import session;
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
  if (!s.target_ship().value) {
    g.out << "The hopper seems to be blocked.\n";
    return;
  }

  try {
    g.entity_manager.mutate_ship(s.target_ship(), [&](Ship& s2) {
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

void report_alien_sector_assault(
    GameObj& g, const Race& race, const Race& alien, const Ship& ship,
    const Sector& sect, PopulationType what, population_t surviving_attackers,
    player_t attacker_player, governor_t attacker_gov, player_t defender_owner,
    governor_t defender_gov, population_t attacker_casualties,
    population_t defender_civ_casualties,
    population_t defender_mil_casualties) {
  const auto& star = *g.entity_manager.peek_star(g.snum());
  bool attacker_won = (sect.get_owner() == attacker_player);

  std::string telegram = std::format(
      "/{}/{}: {} [{}] {} assaults {} [{}] {}({}) {}\n", star.get_name(),
      star.get_planet_name(g.pnum()), race.name, attacker_player, ship,
      alien.name, alien.Playernum, sect.condition_symbol(), ship.land_coords(),
      attacker_won ? "VICTORY" : "DEFEAT");

  if (attacker_won) {
    g.out << "VICTORY! The sector is yours!\n";
    telegram += "Sector CAPTURED!\n";
    if (surviving_attackers > 0) {
      g.out << std::format("{} {} move in.\n", surviving_attackers,
                           what == PopulationType::CIV ? "civilians"
                                                       : "troops");
    }
  } else {
    g.out << "DEFEAT!  Your assault was repulsed.\n";
    telegram += "Assault repulsed!\n";
  }

  telegram += std::format("Casualties: Yours: {} mil/{} civ    Theirs: {} {}\n",
                          defender_mil_casualties, defender_civ_casualties,
                          attacker_casualties,
                          what == PopulationType::MIL ? "mil" : "civ");
  g.out << std::format(
      "Crew casualties: Yours: {} {}    Theirs: {} mil/{} civ\n",
      attacker_casualties, what == PopulationType::MIL ? "mil" : "civ",
      defender_mil_casualties, defender_civ_casualties);

  warn_player(g.session_registry, g.entity_manager, defender_owner,
              defender_gov, telegram);

  auto news = std::format("/{}/{}: {} [{}] {} {} by {} [{}] on sector {}.\n",
                          star.get_name(), star.get_planet_name(g.pnum()),
                          race.name, attacker_player, ship,
                          attacker_won ? "CAPTURED" : "failed to capture",
                          alien.name, alien.Playernum, ship.land_coords());
  post(g.entity_manager, news, NewsType::COMBAT);
  notify_star(g.session_registry, g.entity_manager, attacker_player,
              attacker_gov, g.snum(), news);
}

void resolve_alien_sector_combat(GameObj& g, Race& race, Race& alien,
                                 Ship& ship, Sector& sect, PopulationType what,
                                 population_t people, player_t attacker_player,
                                 governor_t attacker_gov,
                                 player_t defender_owner,
                                 governor_t defender_gov) {
  population_t initial_attacker_popn = people;
  population_t initial_defender_civ = sect.get_popn();
  population_t initial_defender_mil = sect.get_troops();

  const auto outcome = ground_attack({
      .attacker = race,
      .defender = alien,
      .attacker_force = people,
      .attacker_type = what,
      .defender_civ = sect.get_popn(),
      .defender_mil = sect.get_troops(),
      .attacker_defense_bonus = static_cast<int>(ship.armor()),
      .defender_defense_bonus = sect.defense_bonus(),
      .attacker_compatibility =
          percent_to_fraction(100.0 - static_cast<double>(ship.damage())),
      .defender_compatibility = alien.sector_compatibility(sect),
  });
  people = outcome.surviving_attackers;
  sect.set_popn_exact(outcome.surviving_defender_civ);
  sect.set_troops(outcome.surviving_defender_mil);

  g.session_registry.notify_player(
      attacker_player, attacker_gov,
      std::format("Attack: {:.2f}   Defense: {:.2f}.\n",
                  outcome.attack_strength, outcome.defense_strength));

  if (sect.is_empty()) {
    int absorbed = 0;
    if (race.absorb) {
      absorbed = int_rand(0, initial_defender_civ + initial_defender_mil);
      g.out << std::format("{} alien bodies absorbed.\n", absorbed);
      g.session_registry.notify_player(
          defender_owner, defender_gov,
          std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
    }
    if (what == PopulationType::CIV) {
      sect.set_popn_exact(people + absorbed);
    } else if (what == PopulationType::MIL) {
      sect.set_popn_exact(absorbed);
      sect.set_troops(people);
    }
    sect.set_owner(attacker_player);
    race.adjust_morale(alien, static_cast<int>(alien.fighters));
  } else {
    if (alien.absorb) {
      int absorbed = int_rand(0, initial_attacker_popn - people);
      g.session_registry.notify_player(
          defender_owner, defender_gov,
          std::format("{} alien bodies absorbed.\n", absorbed));
      g.out << std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed);
      sect.add_popn(absorbed);
    }
    g.out << std::format("Loading {} {}\n", people,
                         what == PopulationType::CIV ? "civ" : "mil");
    if (what == PopulationType::CIV) {
      ship.popn() += people;
    } else {
      ship.troops() += people;
    }
    ship.set_mass(ship.mass() + people * race.mass);
    alien.adjust_morale(race, static_cast<int>(race.fighters));
  }

  report_alien_sector_assault(
      g, race, alien, ship, sect, what, people, attacker_player, attacker_gov,
      defender_owner, defender_gov, outcome.attacker_casualties,
      outcome.defender_civ_casualties, outcome.defender_mil_casualties);
}

void unload_onto_alien_sector(GameObj& g, Planet& planet, SectorMap& smap,
                              Ship& ship, Sector& sect, PopulationType what,
                              population_t people) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (people <= 0) {
    g.out << "You have to unload to assault alien sectors.\n";
    return;
  }
  player_t defender_owner = sect.get_owner();

  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    g.entity_manager.mutate_race(defender_owner, [&](Race& alien) {
      g.entity_manager.mutate_star(
          g.snum(), [&](Star& s) { s.record_ground_assault(race, alien); });

      alien.increase_translation(Playernum);
      race.increase_translation(defender_owner);

      const auto& star = *g.entity_manager.peek_star(g.snum());
      governor_t defender_gov = star.governor(defender_owner);

      if (what == PopulationType::CIV) {
        ship.popn() -= people;
      } else {
        ship.troops() -= people;
      }
      ship.set_mass(ship.mass() - people * race.mass);
      g.out << std::format("{} {} unloaded...\n", people,
                           what == PopulationType::CIV ? "civ" : "mil");
      g.out << std::format("Crew compliment {} civ  {} mil\n", ship.popn(),
                           ship.troops());
      g.out << std::format("{} {} assault {} civ/{} mil\n", people,
                           what == PopulationType::CIV ? "civ" : "mil",
                           sect.get_popn(), sect.get_troops());

      resolve_alien_sector_combat(g, race, alien, ship, sect, what, people,
                                  Playernum, Governor, defender_owner,
                                  defender_gov);
    });
  });

  planet.sync_demographics(smap);
}

struct DockingContext {
  bool is_docked_to_ship{false};
  bool is_different_owner{false};
};

std::optional<DockingContext> validate_ship_docking(Ship& s, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (!s.active()) {
    g.session_registry.notify_player(
        Playernum, Governor,
        std::format("{} is irradiated and inactive.\n", s));
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

  if (s.destshipno() == 0) {
    g.out << std::format("{} is not docked.\n", s);
    return std::nullopt;
  }

  std::optional<DockingContext> ctx;
  try {
    g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
      bool mutually_docked = (s.whatorbits() == ScopeLevel::LEVEL_SHIP ||
                              s2.destshipno() == s.number());
      if (!s2.alive() || !mutually_docked) {
        s.launch_to_orbit(s.whatorbits());
        s.whatdest() = ScopeLevel::LEVEL_UNIV;
        g.out << std::format("{} is not docked.\n", s2);
        return;
      }
      if (s2.is_overloaded() && s2.whatorbits() == ScopeLevel::LEVEL_SHIP) {
        g.out << std::format("{} is overloaded!\n", s2);
        return;
      }
      g.out << std::format("{} docked with {}\n", s, s2);
      bool diff = (s2.owner() != Playernum);
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

struct TransferLimits {
  std::int64_t lolim{0};
  std::int64_t uplim{0};
  bool valid_commodity{true};
};

TransferLimits compute_ship_transfer_limits(const Ship& s, const Ship& s2,
                                            char commod, bool diff) {
  TransferLimits lim{};
  switch (commod) {
    case 'x':
    case '&':
      lim.uplim =
          diff ? 0
               : std::min<std::int64_t>(
                     s2.crystals(), s.max_crystals_capacity() - s.crystals());
      lim.lolim =
          diff ? 0
               : -std::min<std::int64_t>(
                     s.crystals(), s2.max_crystals_capacity() - s2.crystals());
      break;
    case 'c':
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(
                             s2.popn(), s.max_crew_capacity() - s.popn());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(
                             s.popn(), s2.max_crew_capacity() - s2.popn());
      break;
    case 'm':
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(s2.troops(),
                                                s.available_mil() - s.troops());
      lim.lolim = diff ? 0
                       : -std::min<std::int64_t>(
                             s.troops(), s2.available_mil() - s2.troops());
      break;
    case 'd':
      lim.uplim =
          diff ? 0
               : std::min<std::int64_t>(
                     s2.destruct(), s.max_destruct_capacity() - s.destruct());
      lim.lolim = -std::min<std::int64_t>(
          s.destruct(), s2.max_destruct_capacity() - s2.destruct());
      break;
    case 'f':
      lim.uplim = diff ? 0
                       : std::min<std::int64_t>(s2.fuel_units(),
                                                s.available_fuel_capacity());
      lim.lolim =
          -std::min<std::int64_t>(s.fuel_units(), s2.available_fuel_capacity());
      break;
    case 'r':
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
    default:
      lim.valid_commodity = false;
      break;
  }
  return lim;
}

TransferLimits compute_planet_transfer_limits(const Ship& s, player_t Playernum,
                                              char commod, GameObj& g) {
  TransferLimits lim{};
  switch (commod) {
    case 'x':
    case '&':
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        lim.uplim =
            std::min<std::int64_t>(p.info(Playernum).crystals,
                                   s.max_crystals_capacity() - s.crystals());
        lim.lolim = -s.crystals();
      });
      break;
    case 'c':
      g.entity_manager.with_sectormap(
          g.snum(), g.pnum(), [&](const SectorMap& smap) {
            const auto& sect = smap.get(s.land_coords());
            lim.uplim = std::min<std::int64_t>(
                sect.get_popn(), s.max_crew_capacity() - s.popn());
            lim.lolim = -s.popn();
          });
      break;
    case 'm':
      g.entity_manager.with_sectormap(
          g.snum(), g.pnum(), [&](const SectorMap& smap) {
            const auto& sect = smap.get(s.land_coords());
            lim.uplim = std::min<std::int64_t>(sect.get_troops(),
                                               s.available_mil() - s.troops());
            lim.lolim = -s.troops();
          });
      break;
    case 'd':
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        lim.uplim =
            std::min<std::int64_t>(p.info(Playernum).destruct,
                                   s.max_destruct_capacity() - s.destruct());
        lim.lolim = -s.destruct();
      });
      break;
    case 'f':
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        lim.uplim = std::min<std::int64_t>(p.info(Playernum).fuel,
                                           s.available_fuel_capacity());
        lim.lolim = -s.fuel_units();
      });
      break;
    case 'r':
      g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& p) {
        if (s.can_strap_cargo_to_hull()) {
          lim.uplim = p.info(Playernum).resource;
        } else {
          lim.uplim =
              std::min<std::int64_t>(p.info(Playernum).resource,
                                     s.max_resource_capacity() - s.resource());
        }
        lim.lolim = -s.resource();
      });
      break;
    default:
      lim.valid_commodity = false;
      break;
  }
  return lim;
}

void print_post_transfer_message(const Ship& s, char commod, std::int64_t amt,
                                 bool transfer_crew, GameObj& g) {
  switch (commod) {
    case 'c':
      if (transfer_crew) {
        g.out << std::format("crew complement of {} is now {}.\n", s, s.popn());
      }
      break;
    case 'm':
      if (transfer_crew) {
        g.out << std::format("troop complement of {} is now {}.\n", s,
                             s.troops());
      }
      break;
    case 'd':
      g.out << std::format("{} destruct transferred.\n", amt);
      if (!s.max_crew_capacity()) {
        g.out << std::format("\n{} {}\n", s,
                             s.destruct() ? "now boobytrapped."
                                          : "no longer boobytrapped.");
      }
      break;
    case 'x':
    case '&':
      g.out << std::format("{} crystal(s) transferred.\n", amt);
      break;
    case 'f':
      g.out << std::format("{} fuel transferred.\n", amt);
      break;
    case 'r':
      g.out << std::format("{} resources transferred.\n", amt);
      break;
  }
}

void execute_ship_to_ship_transfer(Ship& s, char commod, std::int64_t amt,
                                   const Race& race, GameObj& g) {
  g.entity_manager.mutate_ship(s.destshipno(), [&](Ship& s2) {
    const auto cargo_opt = char_to_ship_cargo(commod);
    if (cargo_opt) {
      if (amt > 0) {
        s2.transfer_cargo_to(s, *cargo_opt, amt, race.mass);
      } else {
        s.transfer_cargo_to(s2, *cargo_opt, -amt, race.mass);
      }
    }

    std::string tele_lines;
    switch (commod) {
      case 'r':
        tele_lines += std::format("{} Resources\n", std::abs(amt));
        break;
      case 'f':
        tele_lines += std::format("{} Fuel\n", std::abs(amt));
        break;
      case 'd':
        tele_lines += std::format("{} Destruct\n", std::abs(amt));
        break;
      case 'x':
      case '&':
        tele_lines += std::format("{} Crystal(s)\n", std::abs(amt));
        break;
      case 'c':
      case 'm':
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

  print_post_transfer_message(s, commod, amt, true, g);
}

bool execute_planet_transfer(Ship& s, char commod, std::int64_t amt,
                             const Race& race, GameObj& g) {
  player_t Playernum = g.player();
  bool assaulted = false;
  bool transfer_crew = false;

  g.entity_manager.mutate_planet_and_sectors(
      g.snum(), g.pnum(), [&](Planet& p, SectorMap& smap) {
        auto& sect = smap.get(s.land_coords());
        switch (commod) {
          case 'c':
          case 'm': {
            PopulationType what =
                (commod == 'c') ? PopulationType::CIV : PopulationType::MIL;
            if (sect.get_owner() != 0 && sect.get_owner() != Playernum) {
              g.out << "That sector is already occupied by another player!\n";
              unload_onto_alien_sector(g, p, smap, s, sect, what, -amt);
              assaulted = true;
              return;
            }
            transfer_crew = true;
            bool was_empty = sect.is_empty();
            population_t civ_delta = (commod == 'c') ? -amt : 0;
            population_t mil_delta = (commod == 'm') ? -amt : 0;
            p.adjust_sector_population(sect, Playernum, civ_delta, mil_delta);
            bool is_now_empty = sect.is_empty();

            if (was_empty && !is_now_empty && amt < 0) {
              g.out << std::format("sector {} {}.\n", s.land_coords(),
                                   commod == 'c' ? "COLONIZED" : "OCCUPIED");
            } else if (!was_empty && is_now_empty) {
              g.out << std::format("sector {} evacuated.\n", s.land_coords());
            }
            break;
          }
          case 'd':
            p.info(Playernum).destruct -= amt;
            break;
          case 'x':
          case '&':
            p.info(Playernum).crystals -= amt;
            break;
          case 'f':
            p.info(Playernum).fuel -= amt;
            break;
          case 'r':
            p.info(Playernum).resource -= amt;
            break;
        }
      });

  if (assaulted) {
    return true;
  }

  switch (commod) {
    case 'c':
      if (transfer_crew) s.add_popn(amt, race.mass);
      break;
    case 'm':
      if (transfer_crew) s.add_troops(amt, race.mass);
      break;
    case 'd':
      s.add_destruct(amt);
      break;
    case 'x':
    case '&':
      s.add_crystals(amt);
      break;
    case 'f':
      s.add_fuel(static_cast<double>(amt));
      break;
    case 'r':
      s.add_resource(amt);
      break;
  }

  print_post_transfer_message(s, commod, amt, transfer_crew, g);
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

  TransferLimits lim{};
  if (dock_ctx->is_docked_to_ship) {
    g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
      lim = compute_ship_transfer_limits(s, s2, commod,
                                         dock_ctx->is_different_owner);
    });
  } else {
    lim = compute_planet_transfer_limits(s, Playernum, commod, g);
  }

  if (!lim.valid_commodity) {
    g.out << "No such commodity valid.\n";
    return false;
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
    execute_ship_to_ship_transfer(s, commod, amt, race, g);
  } else {
    if (execute_planet_transfer(s, commod, amt, race, g)) {
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

  ShipList ships(g);
  for (auto ship_handle : ships) {
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
