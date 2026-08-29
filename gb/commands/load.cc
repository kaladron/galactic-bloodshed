// SPDX-License-Identifier: Apache-2.0

/// \file load.cc
/// \brief Functions for loading and unloading commodities to/from ships.

module commands;

import session;
import gb.entities;
import gb.services;
import notification;
import std;

namespace {
int landed_on(const Ship& s, const shipnum_t shipno) {
  return (s.whatorbits() == ScopeLevel::LEVEL_SHIP && s.destshipno() == shipno);
}

void do_transporter(const Race& race, GameObj& g, Ship* s) {
  player_t Playernum = g.player();

  Playernum = race.Playernum;

  if (!s->is_landed()) {
    g.out << "Origin ship not landed.\n";
    return;
  }
  if (s->storbits() != g.snum() || s->pnumorbits() != g.pnum()) {
    g.out << "Change scope to the planet the ship is landed on!\n";
    return;
  }
  if (s->damage()) {
    g.out << "Origin device is damaged.\n";
    return;
  }
  if (!std::holds_alternative<TransportData>(s->special())) {
    g.out << "Transport device not configured.\n";
    return;
  }
  auto transport = std::get<TransportData>(s->special());

  if (transport.target == 0) {
    g.out << "The hopper seems to be blocked.\n";
    return;
  }

  g.entity_manager.mutate_ship(transport.target, [&](Ship& s2) {
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

    g.out << "Zap\07!\n"; /* ^G */
    /* send stuff to other ship (could be transport device) */
    std::string tele_lines;
    if (s->resource()) {
      rcv_resource(s2, (int)s->resource());
      g.out << std::format("{} resources transferred.\n", s->resource());
      tele_lines += std::format("{} Resources\n", s->resource());
      use_resource(*s, (int)s->resource());
    }
    if (s->fuel()) {
      rcv_fuel(s2, s->fuel());
      g.out << std::format("{} fuel transferred.\n", s->fuel());
      tele_lines += std::format("{} Fuel\n", s->fuel());
      use_fuel(*s, s->fuel());
    }

    if (s->destruct()) {
      rcv_destruct(s2, (int)s->destruct());
      g.out << std::format("{} destruct transferred.\n", s->destruct());
      tele_lines += std::format("{} Destruct\n", s->destruct());
      use_destruct(*s, (int)s->destruct());
    }

    if (s->popn()) {
      s2.mass() += s->popn() * race.mass;
      s2.popn() += s->popn();

      g.out << std::format("{} population transferred.\n", s->popn());
      tele_lines +=
          std::format("{} {}\n", s->popn(),
                      race.Metamorph ? "tons of biomass" : "population");
      s->mass() -= s->popn() * race.mass;
      s->popn() -= s->popn();
    }

    if (s->crystals()) {
      s2.crystals() += s->crystals();

      g.out << std::format("{} crystal(s) transferred.\n", s->crystals());
      tele_lines += std::format("{} crystal(s)\n", s->crystals());

      s->crystals() = 0;
    }

    if (s2.owner() != s->owner()) {
      std::string telegram =
          "Audio-vibatory-physio-molecular transport device #";
      telegram += std::format("{} gave your ship {} the following:\n", *s, s2);
      telegram += tele_lines;
      warn_player(g.session_registry, g.entity_manager, s2.owner(),
                  s2.governor(), telegram);
    }
  });
}

void unload_onto_alien_sector(GameObj& g, Planet& planet, Ship* ship,
                              Sector& sect, PopulationType what,
                              population_t people) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  double astrength;
  double dstrength;
  player_t oldowner;
  governor_t oldgov;
  population_t oldpopn;
  population_t old2popn;
  population_t old3popn;
  population_t casualties;
  population_t casualties2;
  population_t casualties3;
  int absorbed;
  int defense;

  if (people <= 0) {
    g.out << "You have to unload to assault alien sectors.\n";
    return;
  }
  ground_assaults[Playernum.value - 1][sect.get_owner().value - 1]
                 [g.snum().value] += 1;

  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    g.entity_manager.mutate_race(sect.get_owner(), [&](Race& alien) {
      /* races find out about each other */
      alien.translate[Playernum.value - 1] =
          MIN(alien.translate[Playernum.value - 1] + 5, 100);
      race.translate[sect.get_owner().value - 1] =
          MIN(race.translate[sect.get_owner().value - 1] + 5, 100);

      oldowner = sect.get_owner();
      const auto& star = *g.entity_manager.peek_star(g.snum());
      oldgov = star.governor(sect.get_owner());

      if (what == PopulationType::CIV)
        ship->popn() -= people;
      else
        ship->troops() -= people;
      ship->mass() -= people * race.mass;
      g.out << std::format("{} {} unloaded...\n", people,
                           what == PopulationType::CIV ? "civ" : "mil");
      g.out << std::format("Crew compliment {} civ  {} mil\n", ship->popn(),
                           ship->troops());

      g.out << std::format("{} {} assault {} civ/{} mil\n", people,
                           what == PopulationType::CIV ? "civ" : "mil",
                           sect.get_popn(), sect.get_troops());
      oldpopn = people;
      old2popn = sect.get_popn();
      old3popn = sect.get_troops();

      defense = Defensedata[sect.get_condition()];
      auto temp_popn = sect.get_popn();
      auto temp_troops = sect.get_troops();
      ground_attack(race, alien, &people, what, &temp_popn, &temp_troops,
                    (int)ship->armor(), defense,
                    1.0 - (double)ship->damage() / 100.0,
                    alien.likes[sect.get_condition()], &astrength, &dstrength,
                    &casualties, &casualties2, &casualties3);
      sect.set_popn_exact(temp_popn);
      sect.set_troops(temp_troops);
      g.session_registry.notify_player(
          Playernum, Governor,
          std::format("Attack: {:.2f}   Defense: {:.2f}.\n", astrength,
                      dstrength));

      if (sect.is_empty()) { /* we got 'em */
        /* mesomorphs absorb the bodies of their victims */
        absorbed = 0;
        if (race.absorb) {
          absorbed = int_rand(0, old2popn + old3popn);
          g.out << std::format("{} alien bodies absorbed.\n", absorbed);
          g.session_registry.notify_player(
              oldowner, oldgov,
              std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
        }
        if (what == PopulationType::CIV)
          sect.set_popn_exact(people + absorbed);
        else if (what == PopulationType::MIL) {
          sect.set_popn_exact(absorbed);
          sect.set_troops(people);
        }
        sect.set_owner(Playernum);
        adjust_morale(race, alien, (int)alien.fighters);
      } else { /* retreat */
        absorbed = 0;
        if (alien.absorb) {
          absorbed = int_rand(0, oldpopn - people);
          g.session_registry.notify_player(
              oldowner, oldgov,
              std::format("{} alien bodies absorbed.\n", absorbed));
          g.out << std::format("Metamorphs have absorbed {} bodies!!!\n",
                               absorbed);
          sect.add_popn(absorbed);
        }
        /* load them back up */
        g.out << std::format("Loading {} {}\n", people,
                             what == PopulationType::CIV ? "civ" : "mil");
        if (what == PopulationType::CIV)
          ship->popn() += people;
        else
          ship->troops() += people;
        ship->mass() += people * race.mass;
        adjust_morale(alien, race, (int)race.fighters);
      }
      std::string telegram =
          std::format("/{}/{}: {} [{}] {} assaults {} [{}] {}({}) {}\n",
                      star.get_name(), star.get_planet_name(g.pnum()),
                      race.name, Playernum, *ship, alien.name, alien.Playernum,
                      Dessymbols[sect.get_condition()], ship->land_coords(),
                      (sect.get_owner() == Playernum ? "VICTORY" : "DEFEAT"));

      if (sect.get_owner() == Playernum) {
        g.out << "VICTORY! The sector is yours!\n";
        telegram += "Sector CAPTURED!\n";
        if (people) {
          g.out << std::format("{} {} move in.\n", people,
                               what == PopulationType::CIV ? "civilians"
                                                           : "troops");
        }
        planet.info(Playernum).numsectsowned++;
        planet.info(Playernum).mob_points += sect.get_mobilization();
        planet.info(oldowner).numsectsowned--;
        planet.info(oldowner).mob_points -= sect.get_mobilization();
      } else {
        g.out << "DEFEAT!  Your assault was repulsed.\n";
        telegram += "Assault repulsed!\n";
      }

      telegram += std::format(
          "Casualties: Yours: {} mil/{} civ    Theirs: {} {}\n", casualties3,
          casualties2, casualties, what == PopulationType::MIL ? "mil" : "civ");
      g.out << std::format(
          "Crew casualties: Yours: {} {}    Theirs: {} mil/{} civ\n",
          casualties, what == PopulationType::MIL ? "mil" : "civ", casualties3,
          casualties2);
      warn_player(g.session_registry, g.entity_manager, oldowner, oldgov,
                  telegram);
      auto news = std::format(
          "/{}/{}: {} [{}] {} {} by {} [{}] on sector {}.\n", star.get_name(),
          star.get_planet_name(g.pnum()), race.name, Playernum, *ship,
          (sect.get_owner() == Playernum ? "CAPTURED" : "failed to capture"),
          alien.name, alien.Playernum, ship->land_coords());
      post(g.entity_manager, news, NewsType::COMBAT);
      notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                  g.snum(), news);
    });
  });
}
}  // namespace

namespace GB::commands {
bool load(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  int mode = argv[0] == "load" ? 0 : 1;  // load or unload
  char commod;
  bool success = false;

  if (argv.size() < 3) {
    if (mode == 0) {
      g.out << "Load what?\n";
    } else {
      g.out << "Unload what?\n";
    }
    return false;
  }

  ShipList ships(g);
  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;
    unsigned char sh = 0;
    unsigned char diff = 0;
    int lolim = 0;
    int uplim = 0;
    int amt = 0;
    int transfercrew = 0;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;
    if (s.owner() != Playernum || !s.alive()) {
      continue;
    }
    if (!s.active()) {
      g.session_registry.notify_player(
          Playernum, Governor,
          std::format("{} is irradiated and inactive.\n", s));

      continue;
    }
    if (!s.docked()) {
      g.out << std::format("{} is not landed or docked.\n", s);

      continue;
    } /* ship has a recipient */

    if (s.whatdest() == ScopeLevel::LEVEL_PLAN) {
      g.out << std::format("{} at {}\n", s, s.land_coords());
      if (s.storbits() != g.snum() || s.pnumorbits() != g.pnum()) {
        g.out << "Change scope to the planet this ship is landed on.\n";
        continue;
      }
    } else { /* ship is docked */
      if (s.destshipno() == 0) {
        g.out << std::format("{} is not docked.\n", s);
        continue;
      }
      bool is_docked_valid = true;
      try {
        g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
          if (!s2.alive() || !(s.whatorbits() == ScopeLevel::LEVEL_SHIP ||
                               s2.destshipno() == s.number())) {
            /* the ship it was docked with died or
               undocked with it or something. */
            s.docked() = 0;
            s.whatdest() = ScopeLevel::LEVEL_UNIV;

            g.out << std::format("{} is not docked.\n", s2);
            is_docked_valid = false;
            return;
          }
          if (s2.is_overloaded() && s2.whatorbits() == ScopeLevel::LEVEL_SHIP) {
            g.out << std::format("{} is overloaded!\n", s2);
            is_docked_valid = false;
            return;
          }
          g.out << std::format("{} docked with {}\n", s, s2);
          sh = 1;
          if (s2.owner() != Playernum) {
            g.out << std::format("Player {} owns that ship.\n", s2.owner());
            diff = 1;
          }
        });
      } catch (const EntityNotFoundError&) {
        g.out << "Destination ship is bogus.\n";
        continue;
      }
      if (!is_docked_valid) {
        continue;
      }
    }

    commod = argv[2][0];
    if (argv.size() > 3)
      amt = std::stoi(argv[3]);
    else
      amt = 0;

    if (mode) amt = -amt; /* unload */

    if (amt < 0 && s.type() == ShipType::OTYPE_VN) {
      g.out << "You can't unload VNs.\n";
      continue;
    }

    switch (commod) {
      case 'x':
      case '&':
        if (sh) {
          g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
            uplim = diff ? 0
                         : MIN(s2.crystals(),
                               s.max_crystals_capacity() - s.crystals());
            lolim = diff ? 0
                         : -MIN(s.crystals(),
                                s2.max_crystals_capacity() - s2.crystals());
          });
        } else {
          g.entity_manager.with_planet(
              g.snum(), g.pnum(), [&](const Planet& p) {
                uplim = MIN(p.info(Playernum).crystals,
                            s.max_crystals_capacity() - s.crystals());
                lolim = -s.crystals();
              });
        }
        break;
      case 'c':
        if (sh) {
          g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
            uplim = diff ? 0 : MIN(s2.popn(), s.max_crew_capacity() - s.popn());
            lolim =
                diff ? 0 : -MIN(s.popn(), s2.max_crew_capacity() - s2.popn());
          });
        } else {
          g.entity_manager.with_sectormap(
              g.snum(), g.pnum(), [&](const SectorMap& smap) {
                const auto& sect = smap.get(s.land_coords());
                uplim = MIN(sect.get_popn(), s.max_crew_capacity() - s.popn());
                lolim = -s.popn();
              });
        }
        break;
      case 'm':
        if (sh) {
          g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
            uplim = diff ? 0 : MIN(s2.troops(), s.available_mil() - s.troops());
            lolim =
                diff ? 0 : -MIN(s.troops(), s2.available_mil() - s2.troops());
          });
        } else {
          g.entity_manager.with_sectormap(
              g.snum(), g.pnum(), [&](const SectorMap& smap) {
                const auto& sect = smap.get(s.land_coords());
                uplim = MIN(sect.get_troops(), s.available_mil() - s.troops());
                lolim = -s.troops();
              });
        }
        break;
      case 'd':
        if (sh) {
          g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
            uplim = diff ? 0
                         : MIN(s2.destruct(),
                               s.max_destruct_capacity() - s.destruct());
            lolim =
                -MIN(s.destruct(), s2.max_destruct_capacity() - s2.destruct());
          });
        } else {
          g.entity_manager.with_planet(
              g.snum(), g.pnum(), [&](const Planet& p) {
                uplim = MIN(p.info(Playernum).destruct,
                            s.max_destruct_capacity() - s.destruct());
                lolim = -s.destruct();
              });
        }
        break;
      case 'f':
        if (sh) {
          g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
            uplim = diff ? 0
                         : MIN((int)s2.fuel(),
                               (int)s.max_fuel_capacity() - (int)s.fuel());
            lolim = -MIN((int)s.fuel(),
                         (int)s2.max_fuel_capacity() - (int)s2.fuel());
          });
        } else {
          g.entity_manager.with_planet(
              g.snum(), g.pnum(), [&](const Planet& p) {
                uplim = MIN((int)p.info(Playernum).fuel,
                            (int)s.max_fuel_capacity() - (int)s.fuel());
                lolim = -(int)s.fuel();
              });
        }
        break;
      case 'r':
        if (sh) {
          g.entity_manager.with_ship(s.destshipno(), [&](const Ship& s2) {
            if (s.type() == ShipType::STYPE_SHUTTLE &&
                s.whatorbits() != ScopeLevel::LEVEL_SHIP)
              uplim = diff ? 0 : s2.resource();
            else
              uplim = diff ? 0
                           : MIN(s2.resource(),
                                 s.max_resource_capacity() - s.resource());
            if (s2.type() == ShipType::STYPE_SHUTTLE &&
                s.whatorbits() != ScopeLevel::LEVEL_SHIP)
              lolim = -s.resource();
            else
              lolim = -MIN(s.resource(),
                           s2.max_resource_capacity() - s2.resource());
          });
        } else {
          g.entity_manager.with_planet(
              g.snum(), g.pnum(), [&](const Planet& p) {
                uplim = MIN(p.info(Playernum).resource,
                            s.max_resource_capacity() - s.resource());
                lolim = -s.resource();
              });
        }
        break;
      default:
        g.out << "No such commodity valid.\n";
        continue;
    }

    if (amt < lolim || amt > uplim) {
      g.out << std::format("you can only transfer between {} and {}.\n", lolim,
                           uplim);
      continue;
    }

    const auto& race = *g.race;

    if (amt == 0) amt = (mode ? lolim : uplim);

    if (sh) {
      g.entity_manager.mutate_ship(s.destshipno(), [&](Ship& s2) {
        switch (commod) {
          case 'c':
            s2.popn() -= amt;
            if (!landed_on(s, s2.number())) s2.mass() -= amt * race.mass;
            transfercrew = 1;
            break;
          case 'm':
            s2.troops() -= amt;
            if (!landed_on(s, s2.number())) s2.mass() -= amt * race.mass;
            transfercrew = 1;
            break;
          case 'd':
            s2.destruct() -= amt;
            if (!landed_on(s, s2.number())) s2.mass() -= amt * MASS_DESTRUCT;
            break;
          case 'x':
          case '&':
            s2.crystals() -= amt;
            break;
          case 'f':
            s2.fuel() -= (double)amt;
            if (!landed_on(s, s2.number()))
              s2.mass() -= (double)amt * MASS_FUEL;
            break;
          case 'r':
            s2.resource() -= amt;
            if (!landed_on(s, s2.number())) s2.mass() -= amt * MASS_RESOURCE;
            break;
        }

        std::string tele_lines;
        switch (commod) {
          case 'r':
            g.out << std::format("{} resources transferred.\n", amt);
            tele_lines += std::format("{} Resources\n", amt);
            break;
          case 'f':
            g.out << std::format("{} fuel transferred.\n", amt);
            tele_lines += std::format("{} Fuel\n", amt);
            break;
          case 'd':
            g.out << std::format("{} destruct transferred.\n", amt);
            tele_lines += std::format("{} Destruct\n", amt);
            break;
          case 'x':
          case '&':
            g.out << std::format("{} crystals transferred.\n", amt);
            tele_lines += std::format("{} Crystal(s)\n", amt);
            break;
          case 'c':
            g.out << std::format("{} popn transferred.\n", amt);
            tele_lines +=
                std::format("{} {}\n", amt,
                            race.Metamorph ? "tons of biomass" : "population");
            break;
          case 'm':
            g.out << std::format("{} military transferred.\n", amt);
            tele_lines +=
                std::format("{} {}\n", amt,
                            race.Metamorph ? "tons of biomass" : "population");
            break;
        }
        if (!tele_lines.empty()) {
          auto s2_owner = s2.owner();
          auto s2_gov = s2.governor();
          warn_player(
              g.session_registry, g.entity_manager, s2_owner, s2_gov,
              std::format(
                  "Audio-vibatory-physio-molecular transport device #{} gave "
                  "your ship {} the following:\n{}",
                  s, s2, tele_lines));
        }
      });
    } else {
      bool assaulted = false;
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
        switch (commod) {
          case 'c':
            g.entity_manager.mutate_sectormap(
                g.snum(), g.pnum(), [&](SectorMap& smap) {
                  auto& sect = smap.get(s.land_coords());
                  if (sect.get_owner() != 0 && sect.get_owner() != Playernum) {
                    g.out << "That sector is already occupied by another "
                             "player!\n";
                    unload_onto_alien_sector(g, p, &s, sect,
                                             PopulationType::CIV, -amt);
                    assaulted = true;
                    return;
                  }
                  transfercrew = 1;
                  if (!sect.get_popn() && !sect.get_troops() && amt < 0) {
                    p.info(Playernum).numsectsowned++;
                    p.info(Playernum).mob_points += sect.get_mobilization();
                    sect.set_owner(Playernum);
                    g.out << std::format("sector {} COLONIZED.\n",
                                         s.land_coords());
                  }
                  sect.subtract_popn(amt);
                  p.popn() -= amt;
                  p.info(Playernum).popn -= amt;
                  if (!sect.get_popn() && !sect.get_troops()) {
                    p.info(Playernum).numsectsowned--;
                    p.info(Playernum).mob_points -= sect.get_mobilization();
                    sect.set_owner(0);
                    g.out << std::format("sector {} evacuated.\n",
                                         s.land_coords());
                  }
                });
            break;
          case 'm':
            g.entity_manager.mutate_sectormap(
                g.snum(), g.pnum(), [&](SectorMap& smap) {
                  auto& sect = smap.get(s.land_coords());
                  if (sect.get_owner() != 0 && sect.get_owner() != Playernum) {
                    g.out << "That sector is already occupied by another "
                             "player!\n";
                    unload_onto_alien_sector(g, p, &s, sect,
                                             PopulationType::MIL, -amt);
                    assaulted = true;
                    return;
                  }
                  transfercrew = 1;
                  if (sect.is_empty() && amt < 0) {
                    p.info(Playernum).numsectsowned++;
                    p.info(Playernum).mob_points += sect.get_mobilization();
                    sect.set_owner(Playernum);
                    g.out << std::format("sector {} OCCUPIED.\n",
                                         s.land_coords());
                  }
                  sect.set_troops(sect.get_troops() - amt);
                  p.troops() -= amt;
                  p.info(Playernum).troops -= amt;
                  if (sect.is_empty()) {
                    p.info(Playernum).numsectsowned--;
                    p.info(Playernum).mob_points -= sect.get_mobilization();
                    sect.set_owner(0);
                    g.out << std::format("sector {} evacuated.\n",
                                         s.land_coords());
                  }
                });
            break;
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
    }

    switch (commod) {
      case 'c':
        if (transfercrew) {
          s.popn() += amt;
          s.mass() += amt * race.mass;
          g.out << std::format("crew complement of {} is now {}.\n", s,
                               s.popn());
        }
        break;
      case 'm':
        if (transfercrew) {
          s.troops() += amt;
          s.mass() += amt * race.mass;
          g.out << std::format("troop complement of {} is now {}.\n", s,
                               s.troops());
        }
        break;
      case 'd':
        s.destruct() += amt;
        s.mass() += amt * MASS_DESTRUCT;
        g.out << std::format("{} destruct transferred.\n", amt);
        if (!s.max_crew_capacity()) {
          g.out << std::format("\n{} ", s);
          if (s.destruct()) {
            g.out << "now boobytrapped.\n";
          } else {
            g.out << "no longer boobytrapped.\n";
          }
        }
        break;
      case 'x':
      case '&':
        s.crystals() += amt;
        g.out << std::format("{} crystal(s) transferred.\n", amt);
        break;
      case 'f':
        rcv_fuel(s, (double)amt);
        g.out << std::format("{} fuel transferred.\n", amt);
        break;
      case 'r':
        rcv_resource(s, amt);
        g.out << std::format("{} resources transferred.\n", amt);
        break;
    }
    success = true;

    /* do transporting here */
    if (s.type() == ShipType::OTYPE_TRANSDEV && s.on() &&
        std::holds_alternative<TransportData>(s.special()) &&
        std::get<TransportData>(s.special()).target)
      do_transporter(race, g, &s);
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
