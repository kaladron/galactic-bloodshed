// SPDX-License-Identifier: Apache-2.0

/// \file doship.cc
/// \brief Execute single ship turn processing.

module;

import std;

module gblib;

void do_repair(Ship& ship, EntityManager& entity_manager) {
  const auto& state = *entity_manager.peek_server_state();

  double maxrep = REPAIR_RATE / static_cast<double>(state.segments);

  /* stations repair for free, and ships docked with them */
  int cost = [&ship, &maxrep, &entity_manager]() {
    if (Shipdata[ship.type()][ABIL_REPAIR]) {
      return 0;
    }
    // Check if docked with a station
    if (ship.docked() && (ship.whatdest() == ScopeLevel::LEVEL_SHIP ||
                          ship.whatorbits() == ScopeLevel::LEVEL_SHIP)) {
      const auto* dest_ship = entity_manager.peek_ship(ship.destshipno());
      if (dest_ship && dest_ship->type() == ShipType::STYPE_STATION) {
        return 0;
      }
    }
    maxrep *= static_cast<double>(ship.popn()) /
              static_cast<double>(ship.max_crew_capacity());
    return static_cast<int>(0.005 * maxrep * ship.effective_cost());
  }();

  if (cost <= ship.resource()) {
    use_resource(ship, cost);
    int drep = static_cast<int>(maxrep);
    ship.damage() = std::max(0, static_cast<int>(ship.damage()) - drep);
  } else {
    /* use up all of the ships resources */
    int drep = static_cast<int>(maxrep * (static_cast<double>(ship.resource()) /
                                          static_cast<double>(cost)));
    use_resource(ship, ship.resource());
    ship.damage() = std::max(0, static_cast<int>(ship.damage()) - drep);
  }
}

void do_habitat(Ship& ship, EntityManager& entity_manager) {
  const auto& race = *entity_manager.peek_race(ship.owner());

  /* In v5.0+ Habitats make resources out of fuel */
  if (ship.on()) {
    double fuse = ship.fuel() *
                  ((double)ship.popn() / (double)ship.max_crew_capacity()) *
                  (1.0 - .01 * (double)ship.damage());
    auto add = (int)fuse / 20;
    if (ship.resource() + add > ship.max_resource_capacity())
      add = ship.max_resource_capacity() - ship.resource();
    fuse = 20.0 * (double)add;
    rcv_resource(ship, add);
    use_fuel(ship, fuse);

    for (auto nested_ship : ShipList(entity_manager, ship.ships())) {
      if (nested_ship->type() == ShipType::OTYPE_WPLANT)
        rcv_destruct(ship, do_weapon_plant(*nested_ship, entity_manager));
    }
  }

  auto add = round_rand((double)ship.popn() * race.birthrate);
  if (ship.popn() + add > ship.max_crew_capacity())
    add = ship.max_crew_capacity() - ship.popn();
  rcv_popn(ship, add, race.mass);
}

void do_meta_infect(player_t who, starnum_t star, planetnum_t pnum, Planet& p,
                    EntityManager& entity_manager) {
  entity_manager.mutate_sectormap(star, pnum, [&](SectorMap& smap) {
    auto& s = smap.get_random();

    if (s.is_owned()) {
      if (s.get_owner() == who) {
        return;  // Already owned by us
      }
      // Sector owned by someone else - check if we can take it
      const auto& owner_race = *entity_manager.peek_race(s.get_owner());
      double fighters = owner_race.fighters;
      double troops = s.get_troops() * fighters / 50.0;
      if (int_rand(1, 100) <= 100.0 * (1.0 - std::exp(-troops))) {
        return;  // Failed to infect - defenders won
      }
    }

    const auto& who_race = *entity_manager.peek_race(who);

    // Infection succeeds
    p.info(who).explored = 1;
    p.info(who).numsectsowned += 1;
    s.set_troops(0);
    s.set_popn_exact(who_race.number_sexes);
    s.set_owner(who);
    s.set_condition(s.get_type());
    if (POD_TERRAFORM) {
      s.set_condition(who_race.likesbest);
    }
  });
}

int infect_planet(player_t who, starnum_t star, planetnum_t pnum,
                  EntityManager& entity_manager) {
  if (success(SPORE_SUCCESS_RATE)) {
    entity_manager.mutate_planet(star, pnum, [&](Planet& planet) {
      do_meta_infect(who, star, pnum, planet, entity_manager);
    });
    return 1;
  }
  return 0;
}

void do_pod(Ship& ship, EntityManager& entity_manager) {
  auto* pod = ship.as<SporePodShip>();
  if (!pod) {
    return;
  }

  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_STAR: {
      const auto& star = *entity_manager.peek_star(ship.storbits());

      if (pod->temperature() < POD_THRESHOLD) {
        const auto& state = *entity_manager.peek_server_state();
        pod->set_temperature(
            pod->temperature() +
            round_rand((double)star.temperature() / (double)state.segments));
        return;
      }

      auto target_planet = star.get_random_planet_index();
      if (!target_planet.has_value()) {
        std::string telegram = std::format(
            "{} has warmed and exploded at {}\n\tno planets in system; spores "
            "dissipated into the void.\n",
            ship, prin_ship_orbits(entity_manager, ship));
        push_telegram(entity_manager, ship.owner(), ship.governor(), telegram);
        entity_manager.kill_ship(ship.owner(), ship);
        return;
      }

      auto i = *target_planet;
      std::stringstream telegram_buf;
      telegram_buf << std::format("{} has warmed and exploded at {}\n", ship,
                                  prin_ship_orbits(entity_manager, ship));
      if (infect_planet(ship.owner(), ship.storbits(), i, entity_manager)) {
        telegram_buf << std::format("\tmeta-colony established on {}.",
                                    star.get_planet_name(i));
      } else {
        telegram_buf << std::format("\tno spores have survived.");
      }
      push_telegram(entity_manager, ship.owner(), ship.governor(),
                    telegram_buf.str());
      entity_manager.kill_ship(ship.owner(), ship);
      return;
    }

    case ScopeLevel::LEVEL_PLAN: {
      if (pod->decay() < POD_DECAY) {
        const auto& state = *entity_manager.peek_server_state();
        pod->set_decay(pod->decay() + round_rand(1.0 / (double)state.segments));
        return;
      }

      std::string telegram =
          std::format("{} has decayed at {}\n", ship,
                      prin_ship_orbits(entity_manager, ship));
      push_telegram(entity_manager, ship.owner(), ship.governor(), telegram);
      entity_manager.kill_ship(ship.owner(), ship);
      return;
    }

    default:
      // Doesn't apply at Universe or Ship
      return;
  }
}

void do_canister(Ship& ship, EntityManager& entity_manager, TurnStats& stats) {
  if (ship.whatorbits() != ScopeLevel::LEVEL_PLAN || ship.is_landed()) {
    return;
  }

  auto* canist = ship.as<CanisterShip>();
  if (!canist) {
    return;
  }

  canist->set_count(canist->count() + 1);
  if (canist->count() < DISSIPATE) {
    if (stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].temp_add <
        -90)
      stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].temp_add =
          -100;
    else
      stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].temp_add -=
          10;
  } else { /* timer expired; destroy canister */
    entity_manager.kill_ship(ship.owner(), ship);

    std::string telegram =
        std::format("Canister of dust previously covering {} has dissipated.\n",
                    prin_ship_orbits(entity_manager, ship));

    const auto& star = *entity_manager.peek_star(ship.storbits());
    const auto& planet =
        *entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
    for (const Race& race : RaceList::readonly(entity_manager)) {
      if (planet.info(race.Playernum).numsectsowned)
        push_telegram(entity_manager, race.Playernum,
                      star.governor(race.Playernum), telegram);
    }
  }
}

void do_greenhouse(Ship& ship, EntityManager& entity_manager,
                   TurnStats& stats) {
  if (ship.whatorbits() != ScopeLevel::LEVEL_PLAN || ship.is_landed()) {
    return;
  }
  auto* canist = ship.as<CanisterShip>();
  if (!canist) {
    return;
  }

  canist->set_count(canist->count() + 1);
  if (canist->count() < DISSIPATE) {
    if (stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].temp_add >
        90)
      stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].temp_add =
          100;
    else
      stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].temp_add +=
          10;
  } else { /* timer expired; destroy canister */
    entity_manager.kill_ship(ship.owner(), ship);
    std::string telegram =
        std::format("Greenhouse gases at {} have dissipated.\n",
                    prin_ship_orbits(entity_manager, ship));

    const auto& star = *entity_manager.peek_star(ship.storbits());
    const auto& planet =
        *entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
    for (const Race& race : RaceList::readonly(entity_manager)) {
      if (planet.info(race.Playernum).numsectsowned)
        push_telegram(entity_manager, race.Playernum,
                      star.governor(race.Playernum), telegram);
    }
  }
}

void do_mirror(Ship& ship, EntityManager& entity_manager, TurnStats& stats) {
  auto* mirror = ship.as<SpaceMirrorShip>();
  if (!mirror) {
    return;
  }

  switch (mirror->aimed_level()) {
    case ScopeLevel::LEVEL_SHIP: { /* ship aimed at is a legal ship now */
      /* if in the same system */
      entity_manager.mutate_ship(mirror->aimed_ship(), [&](Ship& target) {
        if ((ship.whatorbits() == ScopeLevel::LEVEL_STAR ||
             ship.whatorbits() == ScopeLevel::LEVEL_PLAN) &&
            (target.whatorbits() == ScopeLevel::LEVEL_STAR ||
             target.whatorbits() == ScopeLevel::LEVEL_PLAN) &&
            ship.storbits() == target.storbits() && target.alive()) {
          auto range = std::hypot(ship.xpos() - target.xpos(),
                                  ship.ypos() - target.ypos());
          auto i = int_rand(0, round_rand((2. / ((double)(target.shipbody()))) *
                                          (double)(mirror->intensity()) /
                                          (range / PLORBITSIZE + 1.0)));
          std::stringstream telegram_buf;
          telegram_buf << std::format("{} aimed at {}\n", ship, target);
          target.damage() += i;
          if (i) {
            telegram_buf << std::format("{}% damage done.\n", i);
          }
          if (target.damage() >= 100) {
            telegram_buf << std::format("{} DESTROYED!!!\n", target);
            entity_manager.kill_ship(ship.owner(), target);
          }
          push_telegram(entity_manager, target.owner(), target.governor(),
                        telegram_buf.str());
          push_telegram(entity_manager, ship.owner(), ship.governor(),
                        telegram_buf.str());
        }
      });
      break;
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto& star = *entity_manager.peek_star(ship.storbits());
      const auto& planet =
          *entity_manager.peek_planet(ship.storbits(), mirror->aimed_planet());

      double range = std::hypot(ship.xpos() - (star.xpos() + planet.xpos()),
                                ship.ypos() - (star.ypos() + planet.ypos()));

      int i = range > PLORBITSIZE ? PLORBITSIZE * mirror->intensity() / range
                                  : mirror->intensity();

      i = round_rand(.01 * (100.0 - (double)(ship.damage())) * (double)i);
      stats.Stinfo[ship.storbits().value][mirror->aimed_planet().value]
          .temp_add += i;
      break;
    }
    case ScopeLevel::LEVEL_STAR:
      /* have to be in the same system as the star; otherwise
         it's not too fair.. */
      if (ship.whatorbits() > ScopeLevel::LEVEL_UNIV &&
          mirror->aimed_star() == ship.storbits()) {
        entity_manager.mutate_star(ship.storbits(), [&](Star& star) {
          star.stability() += int_rand(0, 1);
        });
      }
      break;
    case ScopeLevel::LEVEL_UNIV:
      break;
  }
}

void do_god(Ship& ship, EntityManager& entity_manager) {
  /* gods have infinite power.... heh heh heh */
  const auto& race = *entity_manager.peek_race(ship.owner());
  if (race.God) {
    ship.fuel() = ship.max_fuel_capacity();
    ship.destruct() = ship.max_destruct_capacity();
    ship.resource() = ship.max_resource_capacity();
  }
}

constexpr double ap_planet_factor(const Planet& p) {
  double x = p.num_sectors();
  return (AP_FACTOR / (AP_FACTOR + x));
}

double crew_factor(const Ship& ship) {
  int maxcrew = Shipdata[ship.type()][ABIL_MAXCREW];

  if (!maxcrew) return 0.0;
  return ((double)ship.popn() / (double)maxcrew);
}

void do_ap(Ship& ship, EntityManager& entity_manager) {
  /* if landed on planet, change conditions to be like race */
  if (ship.is_landed() && ship.on()) {
    const auto& race = *entity_manager.peek_race(ship.owner());
    entity_manager.mutate_planet(
        ship.storbits(), ship.pnumorbits(), [&](Planet& p) {
          if (ship.fuel() >= 3.0) {
            use_fuel(ship, 3.0);
            for (auto j = RTEMP + 1; j <= OTHER; j++) {
              auto d = round_rand(
                  ap_planet_factor(p) * crew_factor(ship) *
                  (double)(race.conditions[j] -
                           p.conditions(static_cast<Conditions>(j))));
              if (d) p.conditions(static_cast<Conditions>(j)) += d;
            }
          } else if (!ship.notified()) {
            ship.notified() = 1;
            ship.on() = 0;
            msg_OOF(entity_manager, ship);
          }
        });
  }
}

void do_oap(Ship& ship, TurnStats& stats) {
  /* "indimidate" the planet below, for enslavement purposes. */
  if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN)
    stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].intimidated =
        true;
}

void doship(Ship& ship, bool update, EntityManager& entity_manager,
            TurnStats& stats) {
  /*ship is active */
  ship.active() = 1;

  if (ship.owner() == 0) ship.alive() = 0;

  if (ship.alive()) {
    /* repair radiation */
    if (ship.rad()) {
      ship.active() = 1;
      /* irradiated ships are immobile.. */
      /* kill off some people */
      /* check to see if ship is active */
      if (success(ship.rad())) ship.active() = 0;
      if (update) {
        ship.popn() = round_rand(ship.popn() * .80);
        ship.troops() = round_rand(ship.troops() * .80);
        if (ship.rad() >= (int)REPAIR_RATE)
          ship.rad() -= int_rand(0, (int)REPAIR_RATE);
        else
          ship.rad() -= int_rand(0, (int)ship.rad());
      }
    } else
      ship.active() = 1;

    if (!ship.popn() && ship.max_crew_capacity() && !ship.docked())
      ship.whatdest() = ScopeLevel::LEVEL_UNIV;

    // Check for supernova damage
    if (ship.whatorbits() != ScopeLevel::LEVEL_UNIV) {
      const auto& star = *entity_manager.peek_star(ship.storbits());
      if (star.nova_stage() > 0) {
        /* damage ships from supernovae */
        /* Maarten: modified to take into account MOVES_PER_UPDATE */
        const auto& state = *entity_manager.peek_server_state();
        ship.damage() += 5L * star.nova_stage() /
                         ((ship.effective_armor() + 1) * state.segments);
        if (ship.damage() >= 100) {
          entity_manager.kill_ship(ship.owner(), ship);
          return;
        }
      }
    }

    if (ship.type() == ShipType::OTYPE_FACTORY && !ship.on()) {
      const auto& race = *entity_manager.peek_race(ship.owner());
      ship.tech() = race.tech;
    }

    if (ship.active()) moveship(entity_manager, ship, update, 1, 0);

    ship.size() = ship_size(ship); /* for debugging */
    if (ship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      entity_manager.mutate_ship(ship.destshipno(), [&](Ship& ship2) {
        if (ship2.owner() != ship.owner()) {
          ship2.owner() = ship.owner();
          ship2.governor() = ship.governor();
        }
      });
      /* just making sure */
    } else if (ship.whatorbits() != ScopeLevel::LEVEL_UNIV &&
               (ship.popn() || ship.type() == ShipType::OTYPE_PROBE)) {
      /* Though I have often used TWCs for exploring, I don't think it is
       * right
       */
      /* to be able to map out worlds with this type of junk. Either a manned
       * ship, */
      /* or a probe, which is designed for this kind of work.  Maarten */
      stats.StarsInhab[ship.storbits().value] = 1;
      entity_manager.mutate_star(ship.storbits(), [&](Star& star) {
        star.mark_inhabited_by(ship.owner());
        star.mark_explored_by(ship.owner());
      });
      if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
        entity_manager.mutate_planet(
            ship.storbits(), ship.pnumorbits(),
            [&](Planet& planet) { planet.info(ship.owner()).explored = 1; });
      }
    }

    /* add ships, popn to total count to add AP's */
    if (update) {
      stats.Power[ship.owner()].ships_owned++;
      stats.Power[ship.owner()].resource += ship.resource();
      stats.Power[ship.owner()].fuel += ship.fuel();
      stats.Power[ship.owner()].destruct += ship.destruct();
      stats.Power[ship.owner()].popn += ship.popn();
      stats.Power[ship.owner()].troops += ship.troops();
    }

    if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      stats.Sdatanumships[ship.owner()]++;
      stats.Sdatapopns[ship.owner()] += ship.popn();
    } else {
      stats.starnumships[ship.storbits().value][ship.owner()]++;
      /* add popn of ships to popn */
      stats.starpopns[ship.storbits().value][ship.owner()] += ship.popn();
      /* set inhabited for ship */
      /* only if manned or probe.  Maarten */
      if (ship.popn() || ship.type() == ShipType::OTYPE_PROBE) {
        stats.StarsInhab[ship.storbits().value] = 1;
        entity_manager.mutate_star(ship.storbits(), [&](Star& star) {
          star.mark_inhabited_by(ship.owner());
          star.mark_explored_by(ship.owner());
        });
      }
    }

    if (ship.active()) {
      /* bombard the planet */
      if (ship.can_bombard() && ship.bombard() &&
          ship.whatorbits() == ScopeLevel::LEVEL_PLAN &&
          ship.whatdest() == ScopeLevel::LEVEL_PLAN &&
          ship.deststar() == ship.storbits() &&
          ship.destpnum() == ship.pnumorbits()) {
        /* ship bombards planet */
        stats.Stinfo[ship.storbits().value][ship.pnumorbits().value].inhab =
            true;
      }

      /* repair ship by the amount of crew it has */
      /* industrial complexes can repair (robot ships
         and offline factories can't repair) */
      if (ship.damage() && ship.repair_capacity())
        do_repair(ship, entity_manager);

      if (update) switch (ship.type()) { /* do this stuff during updates only*/
          case ShipType::OTYPE_CANIST:
            do_canister(ship, entity_manager, stats);
            break;
          case ShipType::OTYPE_GREEN:
            do_greenhouse(ship, entity_manager, stats);
            break;
          case ShipType::STYPE_MIRROR:
            do_mirror(ship, entity_manager, stats);
            break;
          case ShipType::STYPE_GOD:
            do_god(ship, entity_manager);
            break;
          case ShipType::OTYPE_AP:
            do_ap(ship, entity_manager);
            break;
          case ShipType::OTYPE_VN: /* Von Neumann machine */
          case ShipType::OTYPE_BERS:
            if (auto* auto_ship = ship.as<AutonomousShip>()) {
              if (auto_ship->progenitor() == 0) {
                // TODO(jeffbailey): Why is setting this to 1 correct?
                auto_ship->mind().progenitor = 1;
              }
              do_VN(entity_manager, *auto_ship, stats);
            }
            break;
          case ShipType::STYPE_OAP:
            do_oap(ship, stats);
            break;
          case ShipType::STYPE_HABITAT:
            do_habitat(ship, entity_manager);
            break;
          default:
            break;
        }
      if (ship.type() == ShipType::STYPE_POD) do_pod(ship, entity_manager);
    }
  }
}

void domass(Ship& ship, EntityManager& entity_manager) {
  // Get race mass from EntityManager
  double rmass = 1.0;
  if (ship.owner() != 0) {
    const auto* race = entity_manager.peek_race(ship.owner());
    if (race) {
      rmass = race->mass;
    }
  }

  ship.mass() = 0.0;
  ship.hanger() = 0;
  for (auto nested_ship : ShipList(entity_manager, ship.ships())) {
    domass(*nested_ship, entity_manager); /* recursive call */
    ship.mass() += nested_ship->mass();
    ship.hanger() += nested_ship->size();
  }
  ship.mass() += getmass(ship);
  ship.mass() += (double)(ship.popn() + ship.troops()) * rmass;
  ship.mass() += (double)ship.destruct() * MASS_DESTRUCT;
  ship.mass() += ship.fuel() * MASS_FUEL;
  ship.mass() += (double)ship.resource() * MASS_RESOURCE;
}

void doown(Ship& ship, EntityManager& entity_manager) {
  for (auto nested_ship : ShipList(entity_manager, ship.ships())) {
    doown(*nested_ship, entity_manager); /* recursive call */
    nested_ship->owner() = ship.owner();
    nested_ship->governor() = ship.governor();
  }
}

void domissile(Ship& ship, EntityManager& entity_manager) {
  if (!ship.alive() || ship.owner() == 0) return;
  if (!ship.on() || ship.docked()) return;

  /* check to see if it has arrived at it's destination */
  if (ship.whatdest() == ScopeLevel::LEVEL_PLAN &&
      ship.whatorbits() == ScopeLevel::LEVEL_PLAN &&
      ship.destpnum() == ship.pnumorbits()) {
    entity_manager.mutate_planet(
        ship.storbits(), ship.pnumorbits(), [&](Planet& p) {
          /* check to see if PDNs are present */
          for (const Ship& s : ShipList::readonly(entity_manager, p.ships())) {
            if (s.alive() && s.type() == ShipType::OTYPE_PLANDEF) {
              /* attack the PDN instead */
              ship.whatdest() =
                  ScopeLevel::LEVEL_SHIP; /* move missile to PDN for attack */
              ship.xpos() = s.xpos();
              ship.ypos() = s.ypos();
              ship.destshipno() = s.number();
              return;
            }
          }

          entity_manager.mutate_sectormap(
              ship.storbits(), ship.pnumorbits(), [&](SectorMap& smap) {
                Coordinates bomb_coords = [&]() -> Coordinates {
                  if (const auto* missile = ship.as<MissileShip>()) {
                    if (!missile->is_scatter()) {
                      return Coordinates{
                          missile->impact_coords().x % p.dimensions().x,
                          missile->impact_coords().y % p.dimensions().y};
                    }
                  }
                  return smap.get_random().coords();
                }();

                if (auto result_opt = shoot_ship_to_planet(
                        entity_manager, ship, p, (int)ship.destruct(),
                        bomb_coords, smap, 0, GTYPE_HEAVY)) {
                  push_telegram(entity_manager, ship.owner(), ship.governor(),
                                result_opt->long_message);
                  entity_manager.kill_ship(ship.owner(), ship);
                  std::string sectors_destroyed_msg = std::format(
                      "{} dropped on {}.\n\t{} sectors destroyed.\n", ship,
                      prin_ship_orbits(entity_manager, ship),
                      result_opt->sectors_destroyed);
                  const auto& star = *entity_manager.peek_star(ship.storbits());
                  for (const Race& race : RaceList::readonly(entity_manager)) {
                    if (p.info(race.Playernum).numsectsowned &&
                        race.Playernum != ship.owner()) {
                      push_telegram(entity_manager, race.Playernum,
                                    star.governor(race.Playernum),
                                    sectors_destroyed_msg);
                    }
                  }
                  if (result_opt->sectors_destroyed) {
                    std::string dropmsg =
                        std::format("{} dropped on {}.\n", ship,
                                    prin_ship_orbits(entity_manager, ship));
                    post(entity_manager, dropmsg, NewsType::COMBAT);
                  }
                }
              });
        });
  } else if (ship.whatdest() == ScopeLevel::LEVEL_SHIP) {
    auto sh2 = ship.destshipno();
    entity_manager.mutate_ship(sh2, [&](Ship& target) {
      auto dist =
          std::hypot(ship.xpos() - target.xpos(), ship.ypos() - target.ypos());
      if (dist <= ((double)ship.speed() * STRIKE_DISTANCE_FACTOR *
                   (100.0 - (double)ship.damage()) / 100.0)) {
        /* do the attack */
        auto s2sresult = shoot_ship_to_ship(entity_manager, ship, target,
                                            (int)ship.destruct(), 0);
        auto const& [damage, short_buf, long_buf] = *s2sresult;
        push_telegram(entity_manager, ship.owner(), ship.governor(), long_buf);
        push_telegram(entity_manager, target.owner(), target.governor(),
                      long_buf);
        entity_manager.kill_ship(ship.owner(), ship);
        post(entity_manager, short_buf, NewsType::COMBAT);
      }
    });
  }
}

void domine(Ship& ship, int detonate, EntityManager& entity_manager) {
  if (ship.type() != ShipType::STYPE_MINE || !ship.alive() ||
      ship.owner() == 0) {
    return;
  }

  /* check around and see if we should explode. */
  if (!ship.on() && !detonate) {
    return;
  }

  if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV ||
      ship.whatorbits() == ScopeLevel::LEVEL_SHIP)
    return;

  auto sh = [&ship, &entity_manager] -> shipnum_t {
    if (ship.whatorbits() == ScopeLevel::LEVEL_STAR) {
      const auto& star = *entity_manager.peek_star(ship.storbits());
      return star.ships();
    } else {  // ScopeLevel::LEVEL_PLAN
      const auto& planet =
          *entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
      return planet.ships();
    }
  }();

  // traverse the list, look for ships that are closer than the trigger
  // radius.
  bool rad = false;
  if (!detonate) {
    const auto& race = *entity_manager.peek_race(ship.owner());

    const ShipList kShiplist(entity_manager, sh);
    for (const Ship& s : kShiplist) {
      double xd = s.xpos() - ship.xpos();
      double yd = s.ypos() - ship.ypos();
      double range = std::hypot(xd, yd);
      if (const auto* mine = ship.as<MineShip>()) {
        if (!race.is_allied_with(s.owner()) && (s.owner() != ship.owner()) &&
            (range <= static_cast<double>(mine->trigger_radius()))) {
          rad = true;
          break;
        }
      }
    }
  } else {
    rad = true;
  }

  if (!rad) {
    return;
  }

  std::string postmsg = std::format("{} detonated at {}\n", ship,
                                    prin_ship_orbits(entity_manager, ship));
  post(entity_manager, postmsg, NewsType::COMBAT);
  telegram_star(entity_manager, ship.storbits(), ship.owner(), ship.governor(),
                postmsg);
  ShipList shiplist(entity_manager, sh);
  for (auto ship_handle : shiplist) {
    Ship& s = *ship_handle;
    if (s.number() != ship.number() && s.alive() &&
        (s.type() != ShipType::OTYPE_CANIST) &&
        (s.type() != ShipType::OTYPE_GREEN)) {
      auto s2sresult = shoot_ship_to_ship(entity_manager, ship, s,
                                          (int)(ship.destruct()), 0, false);
      if (s2sresult) {
        auto const& [damage, short_buf, long_buf] = *s2sresult;
        post(entity_manager, short_buf, NewsType::COMBAT);
        push_telegram(entity_manager, s.owner(), s.governor(), long_buf);
      }
      // Explicitly save the modified ship
      ship_handle.save();
    }
  }

  /* if the mine is in orbit around a planet, nuke the planet too! */
  if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    /* pick a random sector to nuke */
    entity_manager.mutate_planet(
        ship.storbits(), ship.pnumorbits(), [&](Planet& planet) {
          entity_manager.mutate_sectormap(
              ship.storbits(), ship.pnumorbits(), [&](SectorMap& smap) {
                const Coordinates target_coords =
                    ship.is_landed() ? ship.land_coords()
                                     : smap.get_random().coords();

                if (auto result_opt = shoot_ship_to_planet(
                        entity_manager, ship, planet, (int)(ship.destruct()),
                        target_coords, smap, 0, GTYPE_LIGHT)) {
                  std::stringstream telegram;
                  telegram << postmsg;
                  if (result_opt->sectors_destroyed > 0) {
                    telegram << std::format(" - {} sectors destroyed.",
                                            result_opt->sectors_destroyed);
                  }
                  telegram << "\n";

                  const auto& star = *entity_manager.peek_star(ship.storbits());
                  for (const Race& race : RaceList::readonly(entity_manager)) {
                    if (result_opt->nuked_players[race.Playernum]) {
                      push_telegram(entity_manager, race.Playernum,
                                    star.governor(race.Playernum),
                                    telegram.str());
                    }
                  }
                  push_telegram(entity_manager, ship.owner(), ship.governor(),
                                telegram.str());
                }
              });
        });
  }

  entity_manager.kill_ship(ship.owner(), ship);
}

void doabm(Ship& ship, EntityManager& entity_manager) {
  if (!ship.alive() || ship.owner() == 0) return;
  if (!ship.on() || !ship.retaliate() || !ship.destruct()) return;

  if (ship.is_landed()) {
    const auto& planet =
        *entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
    const auto& owner_race = *entity_manager.peek_race(ship.owner());

    /* check to see if missiles/mines are present */
    for (auto target_handle : ShipList(entity_manager, planet.ships())) {
      if (!ship.destruct()) break;  // Exit if out of destruct

      Ship& target = *target_handle;
      if (!target.alive()) continue;
      if (target.type() != ShipType::STYPE_MISSILE &&
          target.type() != ShipType::STYPE_MINE)
        continue;
      if (target.owner() == ship.owner()) continue;

      // Check alliance status
      const auto& target_race = *entity_manager.peek_race(target.owner());
      if (owner_race.is_allied_with(target.owner()) &&
          target_race.is_allied_with(ship.owner())) {
        /* mutually allied missiles don't get shot up */
        continue;
      }

      /* attack the missile/mine */
      auto numdest = retal_strength(ship);
      numdest = MIN(numdest, ship.destruct());
      numdest = MIN(numdest, ship.retaliate());
      ship.destruct() -= numdest;
      auto const& s2sresult =
          shoot_ship_to_ship(entity_manager, ship, target, numdest, 0);
      if (s2sresult) {
        auto [damage, short_buf, long_buf] = *s2sresult;
        push_telegram(entity_manager, ship.owner(), ship.governor(), long_buf);
        push_telegram(entity_manager, target.owner(), target.governor(),
                      long_buf);
        post(entity_manager, short_buf, NewsType::COMBAT);
      }
      target_handle.save();
    }
  }
}

int do_weapon_plant(Ship& ship, EntityManager& entity_manager) {
  const auto& race = *entity_manager.peek_race(ship.owner());
  double tech = race.tech;
  auto maxrate = (int)(tech / 2.0);

  auto rate = round_rand(MIN((double)ship.resource() / (double)RES_COST_WPLANT,
                             ship.fuel() / FUEL_COST_WPLANT) *
                         (1. - .01 * (double)ship.damage()) *
                         (double)ship.popn() / (double)ship.max_crew());
  rate = std::min(rate, maxrate);
  use_resource(ship, (rate * RES_COST_WPLANT));
  use_fuel(ship, ((double)rate * FUEL_COST_WPLANT));
  return rate;
}
