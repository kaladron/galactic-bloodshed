// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace {
int landed_on(const Ship& s, const shipnum_t shipno) {
  return (s.whatorbits() == ScopeLevel::LEVEL_SHIP && s.destshipno() == shipno);
}

void do_transporter(const Race& race, GameObj& g, Ship* s) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  Playernum = race.Playernum;

  if (!landed(*s)) {
    g.out << "Origin ship not landed.\n";
    return;
  }
  if (s->storbits() != g.snum || s->pnumorbits() != g.pnum) {
    notify(Playernum, Governor,
           "Change scope to the planet the ship is landed on!\n");
    return;
  }
  if (s->damage()) {
    g.out << "Origin device is damaged.\n";
    return;
  }
  if (!std::holds_alternative<TransportData>(s->special())) {
    notify(Playernum, Governor, "Transport device not configured.\n");
    return;
  }
  auto transport = std::get<TransportData>(s->special());

  auto s2_handle = g.entity_manager.get_ship(transport.target);
  if (!s2_handle.get()) {
    notify(Playernum, Governor, "The hopper seems to be blocked.\n");
    return;
  }
  auto& s2 = *s2_handle;

  if (!s2.alive() || s2.type() != ShipType::OTYPE_TRANSDEV || !s2.on()) {
    notify(Playernum, Governor, "The target device is not receiving.\n");
    return;
  }
  if (!landed(s2)) {
    g.out << "Target ship not landed.\n";
    return;
  }
  if (s2.damage()) {
    g.out << "Target device is damaged.\n";
    return;
  }

  notify(Playernum, Governor, "Zap\07!\n"); /* ^G */
  /* send stuff to other ship (could be transport device) */
  std::string tele_lines;
  if (s->resource()) {
    rcv_resource(s2, (int)s->resource());
    notify(Playernum, Governor,
           std::format("{} resources transferred.\n", s->resource()));
    tele_lines += std::format("{} Resources\n", s->resource());
    use_resource(*s, (int)s->resource());
  }
  if (s->fuel()) {
    rcv_fuel(s2, s->fuel());
    notify(Playernum, Governor,
           std::format("{} fuel transferred.\n", s->fuel()));
    tele_lines += std::format("{} Fuel\n", s->fuel());
    use_fuel(*s, s->fuel());
  }

  if (s->destruct()) {
    rcv_destruct(s2, (int)s->destruct());
    notify(Playernum, Governor,
           std::format("{} destruct transferred.\n", s->destruct()));
    tele_lines += std::format("{} Destruct\n", s->destruct());
    use_destruct(*s, (int)s->destruct());
  }

  if (s->popn()) {
    s2.mass() += s->popn() * race.mass;
    s2.popn() += s->popn();

    notify(Playernum, Governor,
           std::format("{} population transferred.\n", s->popn()));
    tele_lines +=
        std::format("{} {}\n", s->popn(),
                    race.Metamorph ? "tons of biomass" : "population");
    s->mass() -= s->popn() * race.mass;
    s->popn() -= s->popn();
  }

  if (s->crystals()) {
    s2.crystals() += s->crystals();

    notify(Playernum, Governor,
           std::format("{} crystal(s) transferred.\n", s->crystals()));
    tele_lines += std::format("{} crystal(s)\n", s->crystals());

    s->crystals() = 0;
  }

  if (s2.owner() != s->owner()) {
    std::string telegram = "Audio-vibatory-physio-molecular transport device #";
    telegram += std::format("{} gave your ship {} the following:\n",
                            ship_to_string(*s), ship_to_string(s2));
    telegram += tele_lines;
    warn(s2.owner(), s2.governor(), telegram);
  }
}

void unload_onto_alien_sector(GameObj& g, Planet& planet, Ship* ship,
                              Sector& sect, PopulationType what, int people) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  double astrength;
  double dstrength;
  int oldowner;
  int oldgov;
  int oldpopn;
  int old2popn;
  int old3popn;
  int casualties;
  int casualties2;
  int casualties3;
  int absorbed;
  int defense;

  if (people <= 0) {
    notify(Playernum, Governor,
           "You have to unload to assault alien sectors.\n");
    return;
  }
  ground_assaults[Playernum - 1][sect.get_owner() - 1][g.snum] += 1;

  auto race_handle = g.entity_manager.get_race(Playernum);
  if (!race_handle.get()) {
    notify(Playernum, Governor, "Race not found.\n");
    return;
  }
  auto& race = *race_handle;

  auto alien_handle = g.entity_manager.get_race(sect.get_owner());
  if (!alien_handle.get()) {
    notify(Playernum, Governor, "Alien race not found.\n");
    return;
  }
  auto& alien = *alien_handle;

  /* races find out about each other */
  alien.translate[Playernum - 1] = MIN(alien.translate[Playernum - 1] + 5, 100);
  race.translate[sect.get_owner() - 1] =
      MIN(race.translate[sect.get_owner() - 1] + 5, 100);

  oldowner = (int)sect.get_owner();
  const auto& star = *g.entity_manager.peek_star(g.snum);
  oldgov = star.governor(sect.get_owner() - 1);

  if (what == PopulationType::CIV)
    ship->popn() -= people;
  else
    ship->troops() -= people;
  ship->mass() -= people * race.mass;
  notify(Playernum, Governor,
         std::format("{} {} unloaded...\n", people,
                     what == PopulationType::CIV ? "civ" : "mil"));
  notify(Playernum, Governor,
         std::format("Crew compliment {} civ  {} mil\n", ship->popn(),
                     ship->troops()));

  notify(Playernum, Governor,
         std::format("{} {} assault {} civ/{} mil\n", people,
                     what == PopulationType::CIV ? "civ" : "mil",
                     sect.get_popn(), sect.get_troops()));
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
  sect.set_popn(temp_popn);
  sect.set_troops(temp_troops);
  notify(
      Playernum, Governor,
      std::format("Attack: {:.2f}   Defense: {:.2f}.\n", astrength, dstrength));

  if (sect.is_empty()) { /* we got 'em */
    /* mesomorphs absorb the bodies of their victims */
    absorbed = 0;
    if (race.absorb) {
      absorbed = int_rand(0, old2popn + old3popn);
      notify(Playernum, Governor,
             std::format("{} alien bodies absorbed.\n", absorbed));
      notify(oldowner, oldgov,
             std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
    }
    if (what == PopulationType::CIV)
      sect.set_popn(people + absorbed);
    else if (what == PopulationType::MIL) {
      sect.set_popn(absorbed);
      sect.set_troops(people);
    }
    sect.set_owner(Playernum);
    adjust_morale(race, alien, (int)alien.fighters);
  } else { /* retreat */
    absorbed = 0;
    if (alien.absorb) {
      absorbed = int_rand(0, oldpopn - people);
      notify(oldowner, oldgov,
             std::format("{} alien bodies absorbed.\n", absorbed));
      notify(Playernum, Governor,
             std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
      sect.set_popn(sect.get_popn() + absorbed);
    }
    /* load them back up */
    notify(Playernum, Governor,
           std::format("Loading {} {}\n", people,
                       what == PopulationType::CIV ? "civ" : "mil"));
    if (what == PopulationType::CIV)
      ship->popn() += people;
    else
      ship->troops() += people;
    ship->mass() -= people * race.mass;
    adjust_morale(alien, race, (int)race.fighters);
  }
  std::string telegram = std::format(
      "/{}/{}: {} [{}] {} assaults {} [{}] {}({},{}) {}\n", star.get_name(),
      star.get_planet_name(g.pnum), race.name, Playernum, ship_to_string(*ship),
      alien.name, alien.Playernum, Dessymbols[sect.get_condition()],
      ship->land_x(), ship->land_y(),
      (sect.get_owner() == Playernum ? "VICTORY" : "DEFEAT"));

  if (sect.get_owner() == Playernum) {
    notify(Playernum, Governor, "VICTORY! The sector is yours!\n");
    telegram += "Sector CAPTURED!\n";
    if (people) {
      notify(Playernum, Governor,
             std::format("{} {} move in.\n", people,
                         what == PopulationType::CIV ? "civilians" : "troops"));
    }
    planet.info(Playernum - 1).numsectsowned++;
    planet.info(Playernum - 1).mob_points += sect.get_mobilization();
    planet.info(oldowner - 1).numsectsowned--;
    planet.info(oldowner - 1).mob_points -= sect.get_mobilization();
  } else {
    notify(Playernum, Governor, "The invasion was repulsed; try again.\n");
    telegram += "You fought them off!\n";
  }
  if (!(sect.get_popn() + sect.get_troops() + people)) {
    telegram += "You killed all of them!\n";
    /* increase modifier */
    race.translate[oldowner - 1] = MIN(race.translate[oldowner - 1] + 5, 100);
  }
  if (!people) {
    notify(Playernum, Governor,
           "Oh no! They killed your party to the last man!\n");
    /* increase modifier */
    alien.translate[Playernum - 1] =
        MIN(alien.translate[Playernum - 1] + 5, 100);
  }

  telegram += std::format("Casualties: You: {} civ/{} mil, Them: {} {}\n",
                          casualties2, casualties3, casualties,
                          what == PopulationType::CIV ? "civ" : "mil");
  warn(oldowner, oldgov, telegram);
  notify(Playernum, Governor,
         std::format("Casualties: You: {} {}, Them: {} civ/{} mil\n",
                     casualties, what == PopulationType::CIV ? "civ" : "mil",
                     casualties2, casualties3));
}
}  // namespace

namespace GB::commands {
void load(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 0;
  int mode = argv[0] == "load" ? 0 : 1;  // load or unload
  char commod;
  unsigned char sh = 0;
  unsigned char diff = 0;
  int lolim;
  int uplim;
  int amt;
  int transfercrew;

  if (argv.size() < 2) {
    if (mode == 0) {
      g.out << "Load what?\n";
    } else {
      g.out << "Unload what?\n";
    }
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;
    if (s.owner() != Playernum || !s.alive()) {
      continue;
    }
    if (!s.active()) {
      notify(
          Playernum, Governor,
          std::format("{} is irradiated and inactive.\n", ship_to_string(s)));

      continue;
    }
    if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
        continue;
      }
    } else {
      const auto* star = g.entity_manager.peek_star(s.storbits());
      if (!enufAP(Playernum, Governor, star->AP(Playernum - 1), APcount))
        continue;
    }
    if (!s.docked()) {
      notify(Playernum, Governor,
             std::format("{} is not landed or docked.\n", ship_to_string(s)));

      continue;
    } /* ship has a recipient */

    std::optional<EntityHandle<Ship>> s2_handle;
    Ship* s2_ptr = nullptr;

    if (s.whatdest() == ScopeLevel::LEVEL_PLAN) {
      notify(Playernum, Governor,
             std::format("{} at {},{}\n", ship_to_string(s), s.land_x(),
                         s.land_y()));
      if (s.storbits() != g.snum || s.pnumorbits() != g.pnum) {
        notify(Playernum, Governor,
               "Change scope to the planet this ship is landed on.\n");

        continue;
      }
    } else { /* ship is docked */
      if (!s.destshipno()) {
        notify(Playernum, Governor,
               std::format("{} is not docked.\n", ship_to_string(s)));

        continue;
      }
      s2_handle = g.entity_manager.get_ship(s.destshipno());
      if (!s2_handle->get()) {
        g.out << "Destination ship is bogus.\n";

        continue;
      }
      s2_ptr = s2_handle->get();
      if (!s2_ptr->alive() || !(s.whatorbits() == ScopeLevel::LEVEL_SHIP ||
                                s2_ptr->destshipno() == s.number())) {
        /* the ship it was docked with died or
           undocked with it or something. */
        s.docked() = 0;
        s.whatdest() = ScopeLevel::LEVEL_UNIV;

        notify(Playernum, Governor,
               std::format("{} is not docked.\n", ship_to_string(*s2_ptr)));

        continue;
      }
      if (overloaded(*s2_ptr) &&
          s2_ptr->whatorbits() == ScopeLevel::LEVEL_SHIP) {
        notify(Playernum, Governor,
               std::format("{} is overloaded!\n", ship_to_string(*s2_ptr)));

        continue;
      }
      notify(Playernum, Governor,
             std::format("{} docked with {}\n", ship_to_string(s),
                         ship_to_string(*s2_ptr)));
      sh = 1;
      if (s2_ptr->owner() != Playernum) {
        notify(Playernum, Governor,
               std::format("Player {} owns that ship.\n", s2_ptr->owner()));
        diff = 1;
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

    std::optional<EntityHandle<Planet>> planet_handle;
    std::optional<EntityHandle<SectorMap>> sectormap_handle;
    Planet* p_ptr = nullptr;
    Sector* sect_ptr = nullptr;

    if (!sh) {
      planet_handle = g.entity_manager.get_planet(g.snum, g.pnum);
      p_ptr = planet_handle->get();
    }

    if (!sh && (commod == 'c' || commod == 'm')) {
      sectormap_handle = g.entity_manager.get_sectormap(g.snum, g.pnum);
      sect_ptr = &sectormap_handle->get()->get(s.land_x(), s.land_y());
    }

    switch (commod) {
      case 'x':
      case '&':
        if (sh) {
          uplim = diff
                      ? 0
                      : MIN(s2_ptr->crystals(), max_crystals(s) - s.crystals());
          lolim = diff ? 0
                       : -MIN(s.crystals(),
                              max_crystals(*s2_ptr) - s2_ptr->crystals());
        } else {
          uplim = MIN(p_ptr->info(Playernum - 1).crystals,
                      max_crystals(s) - s.crystals());
          lolim = -s.crystals();
        }
        break;
      case 'c':
        if (sh) {
          uplim = diff ? 0 : MIN(s2_ptr->popn(), max_crew(s) - s.popn());
          lolim = diff ? 0 : -MIN(s.popn(), max_crew(*s2_ptr) - s2_ptr->popn());
        } else {
          uplim = MIN(sect_ptr->get_popn(), max_crew(s) - s.popn());
          lolim = -s.popn();
        }
        break;
      case 'm':
        if (sh) {
          uplim = diff ? 0 : MIN(s2_ptr->troops(), max_mil(s) - s.troops());
          lolim =
              diff ? 0 : -MIN(s.troops(), max_mil(*s2_ptr) - s2_ptr->troops());
        } else {
          uplim = MIN(sect_ptr->get_troops(), max_mil(s) - s.troops());
          lolim = -s.troops();
        }
        break;
      case 'd':
        if (sh) {
          uplim = diff
                      ? 0
                      : MIN(s2_ptr->destruct(), max_destruct(s) - s.destruct());
          lolim =
              -MIN(s.destruct(), max_destruct(*s2_ptr) - s2_ptr->destruct());
        } else {
          uplim = MIN(p_ptr->info(Playernum - 1).destruct,
                      max_destruct(s) - s.destruct());
          lolim = -s.destruct();
        }
        break;
      case 'f':
        if (sh) {
          uplim =
              diff ? 0
                   : MIN((int)s2_ptr->fuel(), (int)max_fuel(s) - (int)s.fuel());
          lolim =
              -MIN((int)s.fuel(), (int)max_fuel(*s2_ptr) - (int)s2_ptr->fuel());
        } else {
          uplim = MIN((int)p_ptr->info(Playernum - 1).fuel,
                      (int)max_fuel(s) - (int)s.fuel());
          lolim = -(int)s.fuel();
        }
        break;
      case 'r':
        if (sh) {
          if (s.type() == ShipType::STYPE_SHUTTLE &&
              s.whatorbits() != ScopeLevel::LEVEL_SHIP)
            uplim = diff ? 0 : s2_ptr->resource();
          else
            uplim =
                diff ? 0
                     : MIN(s2_ptr->resource(), max_resource(s) - s.resource());
          if (s2_ptr->type() == ShipType::STYPE_SHUTTLE &&
              s.whatorbits() != ScopeLevel::LEVEL_SHIP)
            lolim = -s.resource();
          else
            lolim =
                -MIN(s.resource(), max_resource(*s2_ptr) - s2_ptr->resource());
        } else {
          uplim = MIN(p_ptr->info(Playernum - 1).resource,
                      max_resource(s) - s.resource());
          lolim = -s.resource();
        }
        break;
      default:
        g.out << "No such commodity valid.\n";
        continue;
    }

    if (amt < lolim || amt > uplim) {
      notify(Playernum, Governor,
             std::format("you can only transfer between {} and {}.\n", lolim,
                         uplim));
      continue;
    }

    const auto& race = *g.race;

    if (amt == 0) amt = (mode ? lolim : uplim);

    switch (commod) {
      case 'c':
        if (sh) {
          s2_ptr->popn() -= amt;
          if (!landed_on(s, s2_ptr->number()))
            s2_ptr->mass() -= amt * race.mass;
          transfercrew = 1;
        } else if (sect_ptr->get_owner() &&
                   sect_ptr->get_owner() != Playernum) {
          notify(Playernum, Governor,
                 "That sector is already occupied by another player!\n");
          /* fight a land battle */
          unload_onto_alien_sector(g, *p_ptr, &s, *sect_ptr,
                                   PopulationType::CIV, -amt);
          return;
        } else {
          transfercrew = 1;
          if (!sect_ptr->get_popn() && !sect_ptr->get_troops() && amt < 0) {
            p_ptr->info(Playernum - 1).numsectsowned++;
            p_ptr->info(Playernum - 1).mob_points +=
                sect_ptr->get_mobilization();
            sect_ptr->set_owner(Playernum);
            notify(Playernum, Governor,
                   std::format("sector {},{} COLONIZED.\n", s.land_x(),
                               s.land_y()));
          }
          sect_ptr->set_popn(sect_ptr->get_popn() - amt);
          p_ptr->popn() -= amt;
          p_ptr->info(Playernum - 1).popn -= amt;
          if (!sect_ptr->get_popn() && !sect_ptr->get_troops()) {
            p_ptr->info(Playernum - 1).numsectsowned--;
            p_ptr->info(Playernum - 1).mob_points -=
                sect_ptr->get_mobilization();
            sect_ptr->set_owner(0);
            notify(Playernum, Governor,
                   std::format("sector {},{} evacuated.\n", s.land_x(),
                               s.land_y()));
          }
        }
        if (transfercrew) {
          s.popn() += amt;
          s.mass() += amt * race.mass;
          notify(Playernum, Governor,
                 std::format("crew complement of {} is now {}.\n",
                             ship_to_string(s), s.popn()));
        }
        break;
      case 'm':
        if (sh) {
          s2_ptr->troops() -= amt;
          if (!landed_on(s, s2_ptr->number()))
            s2_ptr->mass() -= amt * race.mass;
          transfercrew = 1;
        } else if (sect_ptr->get_owner() &&
                   sect_ptr->get_owner() != Playernum) {
          notify(Playernum, Governor,
                 "That sector is already occupied by another player!\n");
          unload_onto_alien_sector(g, *p_ptr, &s, *sect_ptr,
                                   PopulationType::MIL, -amt);
          return;
        } else {
          transfercrew = 1;
          if (sect_ptr->is_empty() && amt < 0) {
            p_ptr->info(Playernum - 1).numsectsowned++;
            p_ptr->info(Playernum - 1).mob_points +=
                sect_ptr->get_mobilization();
            sect_ptr->set_owner(Playernum);
            notify(Playernum, Governor,
                   std::format("sector {},{} OCCUPIED.\n", s.land_x(),
                               s.land_y()));
          }
          sect_ptr->set_troops(sect_ptr->get_troops() - amt);
          p_ptr->troops() -= amt;
          p_ptr->info(Playernum - 1).troops -= amt;
          if (sect_ptr->is_empty()) {
            p_ptr->info(Playernum - 1).numsectsowned--;
            p_ptr->info(Playernum - 1).mob_points -=
                sect_ptr->get_mobilization();
            sect_ptr->set_owner(0);
            notify(Playernum, Governor,
                   std::format("sector {},{} evacuated.\n", s.land_x(),
                               s.land_y()));
          }
        }
        if (transfercrew) {
          s.troops() += amt;
          s.mass() += amt * race.mass;
          notify(Playernum, Governor,
                 std::format("troop complement of {} is now {}.\n",
                             ship_to_string(s), s.troops()));
        }
        break;
      case 'd':
        if (sh) {
          s2_ptr->destruct() -= amt;
          if (!landed_on(s, s2_ptr->number()))
            s2_ptr->mass() -= amt * MASS_DESTRUCT;
        } else
          p_ptr->info(Playernum - 1).destruct -= amt;

        s.destruct() += amt;
        s.mass() += amt * MASS_DESTRUCT;
        notify(Playernum, Governor,
               std::format("{} destruct transferred.\n", amt));
        if (!max_crew(s)) {
          notify(Playernum, Governor, std::format("\n{} ", ship_to_string(s)));
          if (s.destruct()) {
            notify(Playernum, Governor, "now boobytrapped.\n");
          } else {
            notify(Playernum, Governor, "no longer boobytrapped.\n");
          }
        }
        break;
      case 'x':
        if (sh) {
          s2_ptr->crystals() -= amt;
        } else
          p_ptr->info(Playernum - 1).crystals -= amt;
        s.crystals() += amt;
        notify(Playernum, Governor,
               std::format("{} crystal(s) transferred.\n", amt));
        break;
      case 'f':
        if (sh) {
          s2_ptr->fuel() -= (double)amt;
          if (!landed_on(s, s2_ptr->number()))
            s2_ptr->mass() -= (double)amt * MASS_FUEL;
        } else
          p_ptr->info(Playernum - 1).fuel -= amt;
        rcv_fuel(s, (double)amt);
        notify(Playernum, Governor, std::format("{} fuel transferred.\n", amt));
        break;
      case 'r':
        if (sh) {
          s2_ptr->resource() -= amt;
          if (!landed_on(s, s2_ptr->number()))
            s2_ptr->mass() -= amt * MASS_RESOURCE;
        } else
          p_ptr->info(Playernum - 1).resource -= amt;
        rcv_resource(s, amt);
        notify(Playernum, Governor,
               std::format("{} resources transferred.\n", amt));
        break;
      default:
        g.out << "No such commodity.\n";
        continue;
    }

    if (sh) {
      /* ship to ship transfer */
      std::string tele_lines;
      switch (commod) {
        case 'r':
          notify(Playernum, Governor,
                 std::format("{} resources transferred.\n", amt));
          tele_lines += std::format("{} Resources\n", amt);
          break;
        case 'f':
          notify(Playernum, Governor,
                 std::format("{} fuel transferred.\n", amt));
          tele_lines += std::format("{} Fuel\n", amt);
          break;
        case 'd':
          notify(Playernum, Governor,
                 std::format("{} destruct transferred.\n", amt));
          tele_lines += std::format("{} Destruct\n", amt);
          break;
        case 'x':
        case '&':
          notify(Playernum, Governor,
                 std::format("{} crystals transferred.\n", amt));
          tele_lines += std::format("{} Crystal(s)\n", amt);
          break;
        case 'c':
          notify(Playernum, Governor,
                 std::format("{} popn transferred.\n", amt));
          tele_lines +=
              std::format("{} {}\n", amt,
                          race.Metamorph ? "tons of biomass" : "population");
          break;
        case 'm':
          notify(Playernum, Governor,
                 std::format("{} military transferred.\n", amt));
          tele_lines +=
              std::format("{} {}\n", amt,
                          race.Metamorph ? "tons of biomass" : "population");
          break;
        default:
          break;
      }
      if (!tele_lines.empty()) {
        auto s2_owner = s2_ptr->owner();
        auto s2_gov = s2_ptr->governor();
        auto s2_name = ship_to_string(*s2_ptr);
        warn(s2_owner, s2_gov,
             std::format(
                 "Audio-vibatory-physio-molecular transport device #{} gave "
                 "your ship {} the following:\n{}",
                 ship_to_string(s), s2_name, tele_lines));
      }
    }

    /* do transporting here */
    if (s.type() == ShipType::OTYPE_TRANSDEV && s.on() &&
        std::holds_alternative<TransportData>(s.special()) &&
        std::get<TransportData>(s.special()).target)
      do_transporter(race, g, &s);
  }
}
}  // namespace GB::commands
