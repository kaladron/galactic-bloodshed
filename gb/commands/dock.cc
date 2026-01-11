// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  dock.c -- dock a ship and..... assault -- a very un-PC version of dock */

module;

import session;
import gblib;
import notification;
import std.compat;

#include <strings.h>

module commands;

namespace GB::commands {
void dock(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = (argv[0] == "dock") ? 0 : 1;
  int Assault = (argv[0] == "assault") ? 1 : 0;
  population_t boarders = 0;
  int dam = 0;
  int dam2 = 0;
  int booby = 0;
  PopulationType what;
  shipnum_t ship2no;
  shipnum_t shipno;
  player_t old2owner = 0;
  governor_t old2gov = 0;
  population_t casualties = 0;
  population_t casualties2 = 0;
  population_t casualties3 = 0;
  int casualty_scale = 0;
  double fuel;
  double bstrength = 0;
  double b2strength = 0;
  double Dist;

  if (argv.size() < 3) {
    g.out << "Dock with what?\n";
    return;
  }
  if (argv.size() < 5)
    what = PopulationType::MIL;
  else if (Assault) {
    if (argv[4] == "civilians")
      what = PopulationType::CIV;
    else if (argv[4] == "military")
      what = PopulationType::MIL;
    else {
      g.out << "Assault with what?\n";
      return;
    }
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (Governor != 0 && s.governor() != Governor) continue;

    shipno = s.number();

    if (Assault && s.type() == ShipType::STYPE_POD) {
      g.out << "Sorry. Pods cannot be used to assault.\n";
      continue;
    }
    if (!Assault) {
      if (s.docked() || s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
        g.out << std::format("{} is already docked.\n", ship_to_string(s));
        continue;
      }
    } else if (s.docked()) {
      g.out << "Your ship is already docked.\n";
      continue;
    } else if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      g.out << "Your ship is landed on another ship.\n";
      continue;
    }

    if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      const auto* universe = g.entity_manager.peek_universe();
      if (!enufAP(g.entity_manager, Playernum, Governor,
                  universe->AP[Playernum.value - 1], APcount)) {
        continue;
      }
    } else {
      const auto* star_ptr = g.entity_manager.peek_star(s.storbits());
      if (!star_ptr || !enufAP(g.entity_manager, Playernum, Governor,
                               star_ptr->AP(Playernum), APcount)) {
        continue;
      }
    }

    if (Assault && (what == PopulationType::CIV) && !s.popn()) {
      g.out << "You have no crew on this ship to assault with.\n";
      continue;
    }
    if (Assault && (what == PopulationType::MIL) && !s.troops()) {
      g.out << "You have no troops on this ship to assault with.\n";
      continue;
    }

    auto shiptmp = string_to_shipnum(argv[2]);
    if (!shiptmp) {
      g.out << "Invalid ship number.\n";
    }
    ship2no = *shiptmp;

    if (shipno == ship2no) {
      g.out << "You can't dock with yourself!\n";
      continue;
    }

    auto s2_handle = g.entity_manager.get_ship(ship2no);
    if (!s2_handle.get()) {
      g.out << "The ship wasn't found.\n";
      return;
    }
    Ship& s2 = *s2_handle;

    if (!Assault && testship(s2, g)) {
      g.out << "You are not authorized to do this.\n";
      return;
    }

    /* Check if ships are on same scope level. Maarten */
    if (s.whatorbits() != s2.whatorbits()) {
      g.out << "Those ships are not in the same scope.\n";
      continue;
    }

    if (Assault && (s2.type() == ShipType::OTYPE_VN)) {
      g.out << "You can't assault Von Neumann machines.\n";
      continue;
    }

    if (s2.docked() || (s.whatorbits() == ScopeLevel::LEVEL_SHIP)) {
      g.out << std::format("{} is already docked.\n", ship_to_string(s2));
      continue;
    }

    Dist = std::hypot(s2.xpos() - s.xpos(), s2.ypos() - s.ypos());
    fuel = 0.05 + Dist * 0.025 * (Assault ? 2.0 : 1.0) * sqrt((double)s.mass());

    if (Dist > DIST_TO_DOCK) {
      g.out << std::format("{} must be {:.2f} or closer to {}.\n",
                           ship_to_string(s), DIST_TO_DOCK, ship_to_string(s2));
      continue;
    }
    if (s.docked() && Assault) {
      /* first undock the target ship */
      s.docked() = 0;
      s.whatdest() = ScopeLevel::LEVEL_UNIV;
      auto s3_handle = g.entity_manager.get_ship(s.destshipno());
      if (s3_handle.get()) {
        s3_handle->docked() = 0;
        s3_handle->whatdest() = ScopeLevel::LEVEL_UNIV;
      }
    }

    if (fuel > s.fuel()) {
      g.out << "Not enough fuel.\n";
      continue;
    }
    g.out << std::format("Distance to {}: {:.2f}.\n", ship_to_string(s2), Dist);
    g.out << std::format("This maneuver will take {:.2f} fuel (of {:.2f}.)\n\n",
                         fuel, s.fuel());

    if (s2.docked() && !Assault) {
      g.out << std::format("{} is already docked.\n", ship_to_string(s2));
      return;
    }
    /* defending fire gets defensive fire */
    if (Assault) {
      // Set the command to be distinctive here.  In the target function,
      // APcount is set to 0 and cew is set to 3.
      command_t fire_argv{"fire-from-dock", std::format("#{0}", ship2no),
                          std::format("#{0}", shipno)};
      GB::commands::fire(fire_argv, g);
      if (!s.alive()) {
        continue;
      }
      if (!s2.alive()) {
        return;
      }
    }

    // Get race handles for assault - need them for the entire assault block
    // These are declared here so they're in scope for all assault logic
    // For non-assault, we don't need race handles
    // Use optional since EntityHandle has no default constructor
    std::optional<EntityHandle<Race>> alien_handle_opt;
    std::optional<EntityHandle<Race>> race_handle_opt;
    Race* alien_ptr = nullptr;
    Race* race_ptr = nullptr;

    if (Assault) {
      alien_handle_opt = g.entity_manager.get_race(s2.owner());
      race_handle_opt = g.entity_manager.get_race(Playernum);
      if (!alien_handle_opt->get() || !race_handle_opt->get()) {
        g.out << "Race data not found.\n";
        return;
      }
      alien_ptr = alien_handle_opt->get();
      race_ptr = race_handle_opt->get();
      Race& alien = *alien_ptr;
      Race& race = *race_ptr;

      if (argv.size() >= 4) {
        sscanf(argv[3].c_str(), "%lu", &boarders);
        if ((what == PopulationType::MIL) && (boarders > s.troops()))
          boarders = s.troops();
        else if ((what == PopulationType::CIV) && (boarders > s.popn()))
          boarders = s.popn();
      } else {
        if (what == PopulationType::CIV)
          boarders = s.popn();
        else if (what == PopulationType::MIL)
          boarders = s.troops();
      }
      if (boarders > s2.max_crew()) boarders = s2.max_crew();

      /* Allow assault of crewless ships. */
      if (s2.max_crew() && boarders <= 0) {
        g.out << std::format("Illegal number of boarders ({}).\n", boarders);
        continue;
      }
      old2owner = s2.owner();
      old2gov = s2.governor();
      if (what == PopulationType::MIL)
        s.troops() -= boarders;
      else if (what == PopulationType::CIV)
        s.popn() -= boarders;
      s.mass() -= boarders * race.mass;
      g.out << std::format(
          "Boarding strength :{:.2f}       Defense strength: {:.2f}.\n",
          bstrength = boarders *
                      (what == PopulationType::MIL ? 10 * race.fighters : 1) *
                      .01 * race.tech *
                      morale_factor((double)(race.morale - alien.morale)),
          b2strength = (s2.popn() + 10 * s2.troops() * alien.fighters) * .01 *
                       alien.tech *
                       morale_factor((double)(alien.morale - race.morale)));
    }

    /* the ship moves into position, regardless of success of attack */
    use_fuel(s, fuel);
    s.xpos() = s2.xpos() + int_rand(-1, 1);
    s.ypos() = s2.ypos() + int_rand(-1, 1);
    if (s.hyper_drive().on) {
      s.hyper_drive().on = 0;
      g.out << "Hyper-drive deactivated.\n";
    }
    if (Assault) {
      Race& alien = *alien_ptr;
      Race& race = *race_ptr;

      /* if the assaulted ship is docked, undock it first */
      if (s2.docked() && s2.whatdest() == ScopeLevel::LEVEL_SHIP) {
        auto s3_handle = g.entity_manager.get_ship(s2.destshipno());
        if (s3_handle.get()) {
          s3_handle->docked() = 0;
          s3_handle->whatdest() = ScopeLevel::LEVEL_UNIV;
          s3_handle->destshipno() = 0;
        }

        s2.docked() = 0;
        s2.whatdest() = ScopeLevel::LEVEL_UNIV;
        s2.destshipno() = 0;
      }
      /* nuke both populations, ships */
      casualty_scale = MIN(boarders, s2.troops() + s2.popn());

      if (b2strength) { /* otherwise the ship surrenders */
        casualties =
            int_rand(0, round_rand((double)casualty_scale * (b2strength + 1.0) /
                                   (bstrength + 1.0)));
        casualties = MIN(boarders, casualties);
        boarders -= casualties;

        dam = int_rand(
            0, round_rand(25. * (b2strength + 1.0) / (bstrength + 1.0)));
        dam = MIN(100, dam);
        s.damage() = MIN(100, s.damage() + dam);
        if (s.damage() >= 100) g.entity_manager.kill_ship(Playernum, s);

        casualties2 =
            int_rand(0, round_rand((double)casualty_scale * (bstrength + 1.0) /
                                   (b2strength + 1.0)));
        casualties2 = MIN(s2.popn(), casualties2);
        casualties3 =
            int_rand(0, round_rand((double)casualty_scale * (bstrength + 1.0) /
                                   (b2strength + 1.0)));
        casualties3 = MIN(s2.troops(), casualties3);
        s2.popn() -= casualties2;
        s2.mass() -= casualties2 * alien.mass;
        s2.troops() -= casualties3;
        s2.mass() -= casualties3 * alien.mass;
        /* (their mass) */
        dam2 = int_rand(
            0, round_rand(25. * (bstrength + 1.0) / (b2strength + 1.0)));
        dam2 = MIN(100, dam2);
        s2.damage() = MIN(100, s2.damage() + dam2);
        if (s2.damage() >= 100) g.entity_manager.kill_ship(Playernum, s2);
      } else {
        s2.popn() = 0;
        s2.troops() = 0;
        booby = 0;
        /* do booby traps */
        /* check for boobytrapping */
        if (!s2.max_crew() && s2.destruct())
          booby = int_rand(0, 10 * (int)s2.destruct());
        booby = MIN(100, booby);
      }

      if ((!s2.popn() && !s2.troops()) && s.alive() && s2.alive()) {
        /* we got 'em */
        s.docked() = 1;
        s.whatdest() = ScopeLevel::LEVEL_SHIP;
        s.destshipno() = ship2no;

        s2.docked() = 1;
        s2.whatdest() = ScopeLevel::LEVEL_SHIP;
        s2.destshipno() = shipno;
        old2owner = s2.owner();
        old2gov = s2.governor();
        s2.owner() = s.owner();
        s2.governor() = s.governor();
        if (what == PopulationType::MIL)
          s2.troops() = boarders;
        else
          s2.popn() = boarders;
        s2.mass() += boarders * race.mass; /* our mass */
        if (casualties2 + casualties3) {
          /* You must kill to get morale */
          adjust_morale(race, alien, (int)s2.build_cost());
        }
      } else { /* retreat */
        if (what == PopulationType::MIL)
          s.troops() += boarders;
        else if (what == PopulationType::CIV)
          s.popn() += boarders;
        s.mass() += boarders * race.mass;
        adjust_morale(alien, race, (int)race.fighters);
      }

      /* races find out about each other */
      alien.translate[Playernum.value - 1] =
          MIN(alien.translate[Playernum.value - 1] + 5, 100);
      race.translate[old2owner.value - 1] =
          MIN(race.translate[old2owner.value - 1] + 5, 100);

      if (!boarders && (s2.popn() + s2.troops())) /* boarding party killed */
        alien.translate[Playernum.value - 1] =
            MIN(alien.translate[Playernum.value - 1] + 25, 100);
      if (s2.owner() == Playernum) /* captured ship */
        race.translate[old2owner.value - 1] =
            MIN(race.translate[old2owner.value - 1] + 25, 100);
    } else {
      s.docked() = 1;
      s.whatdest() = ScopeLevel::LEVEL_SHIP;
      s.destshipno() = ship2no;

      s2.docked() = 1;
      s2.whatdest() = ScopeLevel::LEVEL_SHIP;
      s2.destshipno() = shipno;
    }

    if (Assault) {
      std::string telegram = std::format(
          "{} ASSAULTED by {} at {}\n", ship_to_string(s2), ship_to_string(s),
          prin_ship_orbits(g.entity_manager, s2));
      telegram += std::format("Your damage: {}%, theirs: {}%.\n", dam2, dam);
      if (!s2.max_crew() && s2.destruct()) {
        telegram +=
            std::format("(Your boobytrap gave them {}% damage.)\n", booby);
        g.out << std::format("Their boobytrap gave you {}% damage!)\n", booby);
      }
      g.session_registry.notify_player(
          Playernum, Governor,
          std::format("Damage taken:  You: {}% (now {}%)\n", dam, s.damage()));
      if (!s.alive()) {
        g.out << "              YOUR SHIP WAS DESTROYED!!!\n";
        telegram += "              Their ship DESTROYED!!!\n";
      }
      g.out << std::format("              Them: {}% (now {}%)\n", dam2,
                           s2.damage());
      if (!s2.alive()) {
        g.out << "              Their ship DESTROYED!!!  Boarders are dead.\n";
        telegram += "              YOUR SHIP WAS DESTROYED!!!\n";
      }
      if (s.alive()) {
        if (s2.owner() == Playernum) {
          telegram += "CAPTURED!\n";
          g.out << "VICTORY! the ship is yours!\n";
          if (boarders) {
            g.out << std::format("{} boarders move in.\n", boarders);
          }
          capture_stuff(s2, g);
        } else if (s2.popn() + s2.troops()) {
          g.out << "The boarding was repulsed; try again.\n";
          telegram += "You fought them off!\n";
        }
      } else {
        g.out << "The assault was too much for your bucket of bolts.\n";
        telegram += "The assault was too much for their ship..\n";
      }
      if (s2.alive()) {
        if (s2.max_crew() && !boarders) {
          g.out << "Oh no! They killed your boarding party to the last man!\n";
        }
        if (!s.popn() && !s.troops()) {
          telegram += "You killed all their crew!\n";
        }
      } else {
        g.out << "The assault weakened their ship too much!\n";
        telegram += "Your ship was weakened too much!\n";
      }
      telegram += std::format(
          "Casualties: Yours: {} mil/{} civ    Theirs: {} {}\n", casualties3,
          casualties2, casualties, what == PopulationType::MIL ? "mil" : "civ");
      g.out << std::format(
          "Crew casualties: Yours: {} {}    Theirs: {} mil/{} civ\n",
          casualties, what == PopulationType::MIL ? "mil" : "civ", casualties3,
          casualties2);
      warn_player(g.session_registry, g.entity_manager, old2owner, old2gov,
                  telegram);
      auto news = std::format(
          "{} {} {} at {}.\n", ship_to_string(s),
          s2.alive() ? (s2.owner() == Playernum ? "CAPTURED" : "assaulted")
                     : "DESTROYED",
          ship_to_string(s2), prin_ship_orbits(g.entity_manager, s));
      if (s2.owner() == Playernum || !s2.alive())
        post(g.entity_manager, news, NewsType::COMBAT);
      notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                  s.storbits(), news);
    } else {
      g.out << std::format("{} docked with {}.\n", ship_to_string(s),
                           ship_to_string(s2));
    }

    if (g.level() == ScopeLevel::LEVEL_UNIV)
      deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
    else
      deductAPs(g, APcount, g.snum());

    s.notified() = s2.notified() = 0;
  }
}
}  // namespace GB::commands
