// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace {
int landed_on(const Ship& s, const shipnum_t shipno) {
  return (s.whatorbits == ScopeLevel::LEVEL_SHIP && s.destshipno == shipno);
}

void do_transporter(const Race& race, GameObj& g, Ship* s) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  Ship* s2;

  Playernum = race.Playernum;

  if (!landed(*s)) {
    g.out << "Origin ship not landed.\n";
    return;
  }
  if (s->storbits != g.snum || s->pnumorbits != g.pnum) {
    notify(Playernum, Governor,
           "Change scope to the planet the ship is landed on!\n");
    return;
  }
  if (s->damage) {
    g.out << "Origin device is damaged.\n";
    return;
  }
  if (!std::holds_alternative<TransportData>(s->special)) {
    notify(Playernum, Governor, "Transport device not configured.\n");
    return;
  }
  auto transport = std::get<TransportData>(s->special);
  if (!getship(&s2, (int)transport.target)) {
    notify(Playernum, Governor, "The hopper seems to be blocked.\n");
    return;
  }
  if (!s2->alive || s2->type != ShipType::OTYPE_TRANSDEV || !s2->on) {
    notify(Playernum, Governor, "The target device is not receiving.\n");
    free(s2);
    return;
  }
  if (!landed(*s2)) {
    g.out << "Target ship not landed.\n";
    free(s2);
    return;
  }
  if (s2->damage) {
    g.out << "Target device is damaged.\n";
    free(s2);
    return;
  }

  notify(Playernum, Governor, "Zap\07!\n"); /* ^G */
  /* send stuff to other ship (could be transport device) */
  std::string tele_lines;
  if (s->resource) {
    rcv_resource(*s2, (int)s->resource);
    notify(Playernum, Governor,
           std::format("{} resources transferred.\n", s->resource));
    tele_lines += std::format("{} Resources\n", s->resource);
    use_resource(*s, (int)s->resource);
  }
  if (s->fuel) {
    rcv_fuel(*s2, s->fuel);
    notify(Playernum, Governor, std::format("{} fuel transferred.\n", s->fuel));
    tele_lines += std::format("{} Fuel\n", s->fuel);
    use_fuel(*s, s->fuel);
  }

  if (s->destruct) {
    rcv_destruct(*s2, (int)s->destruct);
    notify(Playernum, Governor,
           std::format("{} destruct transferred.\n", s->destruct));
    tele_lines += std::format("{} Destruct\n", s->destruct);
    use_destruct(*s, (int)s->destruct);
  }

  if (s->popn) {
    s2->mass += s->popn * race.mass;
    s2->popn += s->popn;

    notify(Playernum, Governor,
           std::format("{} population transferred.\n", s->popn));
    tele_lines += std::format(
        "{} {}\n", s->popn, race.Metamorph ? "tons of biomass" : "population");
    s->mass -= s->popn * race.mass;
    s->popn -= s->popn;
  }

  if (s->crystals) {
    s2->crystals += s->crystals;

    notify(Playernum, Governor,
           std::format("{} crystal(s) transferred.\n", s->crystals));
    tele_lines += std::format("{} crystal(s)\n", s->crystals);

    s->crystals = 0;
  }

  if (s2->owner != s->owner) {
    std::string telegram = "Audio-vibatory-physio-molecular transport device #";
    telegram += std::format("{} gave your ship {} the following:\n",
                            ship_to_string(*s), ship_to_string(*s2));
    telegram += tele_lines;
    warn(s2->owner, s2->governor, telegram);
  }

  putship(*s2);
  free(s2);
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
  auto& race = races[Playernum - 1];
  auto& alien = races[sect.get_owner() - 1];
  /* races find out about each other */
  alien.translate[Playernum - 1] = MIN(alien.translate[Playernum - 1] + 5, 100);
  race.translate[sect.get_owner() - 1] =
      MIN(race.translate[sect.get_owner() - 1] + 5, 100);

  oldowner = (int)sect.get_owner();
  oldgov = stars[g.snum].governor(sect.get_owner() - 1);

  if (what == PopulationType::CIV)
    ship->popn -= people;
  else
    ship->troops -= people;
  ship->mass -= people * race.mass;
  notify(Playernum, Governor,
         std::format("{} {} unloaded...\n", people,
                     what == PopulationType::CIV ? "civ" : "mil"));
  notify(Playernum, Governor,
         std::format("Crew compliment {} civ  {} mil\n", ship->popn,
                     ship->troops));

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
                (int)ship->armor, defense, 1.0 - (double)ship->damage / 100.0,
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
      ship->popn += people;
    else
      ship->troops += people;
    ship->mass -= people * race.mass;
    adjust_morale(alien, race, (int)race.fighters);
  }
  std::string telegram = std::format(
      "/{}/{}: {} [{}] {} assaults {} [{}] {}({},{}) {}\n",
      stars[g.snum].get_name(), stars[g.snum].get_planet_name(g.pnum),
      race.name, Playernum, ship_to_string(*ship), alien.name, alien.Playernum,
      Dessymbols[sect.get_condition()], ship->land_x, ship->land_y,
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
  putrace(alien);
  putrace(race);

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
  Ship* s2;
  Planet p;
  Sector sect;

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
    if (s.owner != Playernum || !s.alive) {
      continue;
    }
    if (!s.active) {
      notify(
          Playernum, Governor,
          std::format("{} is irradiated and inactive.\n", ship_to_string(s)));

      continue;
    }
    if (s.whatorbits == ScopeLevel::LEVEL_UNIV) {
      if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
        continue;
      }
    } else if (!enufAP(Playernum, Governor, stars[s.storbits].AP(Playernum - 1),
                       APcount))
      continue;
    if (!s.docked) {
      notify(Playernum, Governor,
             std::format("{} is not landed or docked.\n", ship_to_string(s)));

      continue;
    } /* ship has a recipient */
    if (s.whatdest == ScopeLevel::LEVEL_PLAN) {
      notify(
          Playernum, Governor,
          std::format("{} at {},{}\n", ship_to_string(s), s.land_x, s.land_y));
      if (s.storbits != g.snum || s.pnumorbits != g.pnum) {
        notify(Playernum, Governor,
               "Change scope to the planet this ship is landed on.\n");

        continue;
      }
    } else { /* ship is docked */
      if (!s.destshipno) {
        notify(Playernum, Governor,
               std::format("{} is not docked.\n", ship_to_string(s)));

        continue;
      }
      if (!getship(&s2, (int)s.destshipno)) {
        g.out << "Destination ship is bogus.\n";

        continue;
      }
      if (!s2->alive || !(s.whatorbits == ScopeLevel::LEVEL_SHIP ||
                          s2->destshipno == s.number)) {
        /* the ship it was docked with died or
           undocked with it or something. */
        s.docked = 0;
        s.whatdest = ScopeLevel::LEVEL_UNIV;

        notify(Playernum, Governor,
               std::format("{} is not docked.\n", ship_to_string(*s2)));

        free(s2);
        continue;
      }
      if (overloaded(*s2) && s2->whatorbits == ScopeLevel::LEVEL_SHIP) {
        notify(Playernum, Governor,
               std::format("{} is overloaded!\n", ship_to_string(*s2)));

        free(s2);
        continue;
      }
      notify(Playernum, Governor,
             std::format("{} docked with {}\n", ship_to_string(s),
                         ship_to_string(*s2)));
      sh = 1;
      if (s2->owner != Playernum) {
        notify(Playernum, Governor,
               std::format("Player {} owns that ship.\n", s2->owner));
        diff = 1;
      }
    }

    commod = argv[2][0];
    if (argv.size() > 3)
      amt = std::stoi(argv[3]);
    else
      amt = 0;

    if (mode) amt = -amt; /* unload */

    if (amt < 0 && s.type == ShipType::OTYPE_VN) {
      g.out << "You can't unload VNs.\n";

      if (sh) free(s2);
      continue;
    }

    if (!sh) p = getplanet(g.snum, g.pnum);

    if (!sh && (commod == 'c' || commod == 'm'))
      sect = getsector(p, s.land_x, s.land_y);

    switch (commod) {
      case 'x':
      case '&':
        if (sh) {
          uplim = diff ? 0 : MIN(s2->crystals, max_crystals(s) - s.crystals);
          lolim = diff ? 0 : -MIN(s.crystals, max_crystals(*s2) - s2->crystals);
        } else {
          uplim =
              MIN(p.info(Playernum - 1).crystals, max_crystals(s) - s.crystals);
          lolim = -s.crystals;
        }
        break;
      case 'c':
        if (sh) {
          uplim = diff ? 0 : MIN(s2->popn, max_crew(s) - s.popn);
          lolim = diff ? 0 : -MIN(s.popn, max_crew(*s2) - s2->popn);
        } else {
          uplim = MIN(sect.get_popn(), max_crew(s) - s.popn);
          lolim = -s.popn;
        }
        break;
      case 'm':
        if (sh) {
          uplim = diff ? 0 : MIN(s2->troops, max_mil(s) - s.troops);
          lolim = diff ? 0 : -MIN(s.troops, max_mil(*s2) - s2->troops);
        } else {
          uplim = MIN(sect.get_troops(), max_mil(s) - s.troops);
          lolim = -s.troops;
        }
        break;
      case 'd':
        if (sh) {
          uplim = diff ? 0 : MIN(s2->destruct, max_destruct(s) - s.destruct);
          lolim = -MIN(s.destruct, max_destruct(*s2) - s2->destruct);
        } else {
          uplim =
              MIN(p.info(Playernum - 1).destruct, max_destruct(s) - s.destruct);
          lolim = -s.destruct;
        }
        break;
      case 'f':
        if (sh) {
          uplim = diff ? 0 : MIN((int)s2->fuel, (int)max_fuel(s) - (int)s.fuel);
          lolim = -MIN((int)s.fuel, (int)max_fuel(*s2) - (int)s2->fuel);
        } else {
          uplim = MIN((int)p.info(Playernum - 1).fuel,
                      (int)max_fuel(s) - (int)s.fuel);
          lolim = -(int)s.fuel;
        }
        break;
      case 'r':
        if (sh) {
          if (s.type == ShipType::STYPE_SHUTTLE &&
              s.whatorbits != ScopeLevel::LEVEL_SHIP)
            uplim = diff ? 0 : s2->resource;
          else
            uplim = diff ? 0 : MIN(s2->resource, max_resource(s) - s.resource);
          if (s2->type == ShipType::STYPE_SHUTTLE &&
              s.whatorbits != ScopeLevel::LEVEL_SHIP)
            lolim = -s.resource;
          else
            lolim = -MIN(s.resource, max_resource(*s2) - s2->resource);
        } else {
          uplim =
              MIN(p.info(Playernum - 1).resource, max_resource(s) - s.resource);
          lolim = -s.resource;
        }
        break;
      default:
        g.out << "No such commodity valid.\n";
        if (sh) free(s2);

        continue;
    }

    if (amt < lolim || amt > uplim) {
      notify(Playernum, Governor,
             std::format("you can only transfer between {} and {}.\n", lolim,
                         uplim));

      if (sh) free(s2);

      continue;
    }

    auto& race = races[Playernum - 1];

    if (amt == 0) amt = (mode ? lolim : uplim);

    switch (commod) {
      case 'c':
        if (sh) {
          s2->popn -= amt;
          if (!landed_on(s, s2->number)) s2->mass -= amt * race.mass;
          transfercrew = 1;
        } else if (sect.get_owner() && sect.get_owner() != Playernum) {
          notify(Playernum, Governor,
                 "That sector is already occupied by another player!\n");
          /* fight a land battle */
          unload_onto_alien_sector(g, p, &s, sect, PopulationType::CIV, -amt);

          putsector(sect, p, s.land_x, s.land_y);
          putplanet(p, stars[g.snum], g.pnum);

          return;
        } else {
          transfercrew = 1;
          if (!sect.get_popn() && !sect.get_troops() && amt < 0) {
            p.info(Playernum - 1).numsectsowned++;
            p.info(Playernum - 1).mob_points += sect.get_mobilization();
            sect.set_owner(Playernum);
            notify(
                Playernum, Governor,
                std::format("sector {},{} COLONIZED.\n", s.land_x, s.land_y));
          }
          sect.set_popn(sect.get_popn() - amt);
          p.popn() -= amt;
          p.info(Playernum - 1).popn -= amt;
          if (!sect.get_popn() && !sect.get_troops()) {
            p.info(Playernum - 1).numsectsowned--;
            p.info(Playernum - 1).mob_points -= sect.get_mobilization();
            sect.set_owner(0);
            notify(
                Playernum, Governor,
                std::format("sector {},{} evacuated.\n", s.land_x, s.land_y));
          }
        }
        if (transfercrew) {
          s.popn += amt;
          s.mass += amt * race.mass;
          notify(Playernum, Governor,
                 std::format("crew complement of {} is now {}.\n",
                             ship_to_string(s), s.popn));
        }
        break;
      case 'm':
        if (sh) {
          s2->troops -= amt;
          if (!landed_on(s, s2->number)) s2->mass -= amt * race.mass;
          transfercrew = 1;
        } else if (sect.get_owner() && sect.get_owner() != Playernum) {
          notify(Playernum, Governor,
                 "That sector is already occupied by another player!\n");
          unload_onto_alien_sector(g, p, &s, sect, PopulationType::MIL, -amt);

          putsector(sect, p, s.land_x, s.land_y);
          putplanet(p, stars[g.snum], g.pnum);

          return;
        } else {
          transfercrew = 1;
          if (sect.is_empty() && amt < 0) {
            p.info(Playernum - 1).numsectsowned++;
            p.info(Playernum - 1).mob_points += sect.get_mobilization();
            sect.set_owner(Playernum);
            notify(Playernum, Governor,
                   std::format("sector {},{} OCCUPIED.\n", s.land_x, s.land_y));
          }
          sect.set_troops(sect.get_troops() - amt);
          p.troops() -= amt;
          p.info(Playernum - 1).troops -= amt;
          if (sect.is_empty()) {
            p.info(Playernum - 1).numsectsowned--;
            p.info(Playernum - 1).mob_points -= sect.get_mobilization();
            sect.set_owner(0);
            notify(
                Playernum, Governor,
                std::format("sector {},{} evacuated.\n", s.land_x, s.land_y));
          }
        }
        if (transfercrew) {
          s.troops += amt;
          s.mass += amt * race.mass;
          notify(Playernum, Governor,
                 std::format("troop complement of {} is now {}.\n",
                             ship_to_string(s), s.troops));
        }
        break;
      case 'd':
        if (sh) {
          s2->destruct -= amt;
          if (!landed_on(s, s2->number)) s2->mass -= amt * MASS_DESTRUCT;
        } else
          p.info(Playernum - 1).destruct -= amt;

        s.destruct += amt;
        s.mass += amt * MASS_DESTRUCT;
        notify(Playernum, Governor,
               std::format("{} destruct transferred.\n", amt));
        if (!max_crew(s)) {
          notify(Playernum, Governor, std::format("\n{} ", ship_to_string(s)));
          if (s.destruct) {
            notify(Playernum, Governor, "now boobytrapped.\n");
          } else {
            notify(Playernum, Governor, "no longer boobytrapped.\n");
          }
        }
        break;
      case 'x':
        if (sh) {
          s2->crystals -= amt;
        } else
          p.info(Playernum - 1).crystals -= amt;
        s.crystals += amt;
        notify(Playernum, Governor,
               std::format("{} crystal(s) transferred.\n", amt));
        break;
      case 'f':
        if (sh) {
          s2->fuel -= (double)amt;
          if (!landed_on(s, s2->number)) s2->mass -= (double)amt * MASS_FUEL;
        } else
          p.info(Playernum - 1).fuel -= amt;
        rcv_fuel(s, (double)amt);
        notify(Playernum, Governor, std::format("{} fuel transferred.\n", amt));
        break;
      case 'r':
        if (sh) {
          s2->resource -= amt;
          if (!landed_on(s, s2->number)) s2->mass -= amt * MASS_RESOURCE;
        } else
          p.info(Playernum - 1).resource -= amt;
        rcv_resource(s, amt);
        notify(Playernum, Governor,
               std::format("{} resources transferred.\n", amt));
        break;
      default:
        g.out << "No such commodity.\n";

        if (sh) free(s2);

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
        auto s2_owner = s2->owner;
        auto s2_gov = s2->governor;
        auto s2_name = ship_to_string(*s2);
        auto msg = std::format(
            "Audio-vibatory-physio-molecular transport device #{} gave your "
            "ship {} the following:\n{}",
            ship_to_string(s), s2_name, tele_lines);
        warn(s2_owner, s2_gov, msg);
      }
      putship(*s2);
      free(s2);
    } else {
      if (commod == 'c' || commod == 'm') {
        putsector(sect, p, s.land_x, s.land_y);
      }
      putplanet(p, stars[g.snum], g.pnum);
    }

    /* do transporting here */
    if (s.type == ShipType::OTYPE_TRANSDEV && s.on &&
        std::holds_alternative<TransportData>(s.special) &&
        std::get<TransportData>(s.special).target)
      do_transporter(race, g, &s);
  }
}
}  // namespace GB::commands
