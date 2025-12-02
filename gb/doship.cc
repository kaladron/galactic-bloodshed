// SPDX-License-Identifier: Apache-2.0

module;

/* doship -- do one ship turn. */

import std;

module gblib;

namespace {
void do_repair(Ship& ship, EntityManager& entity_manager) {
  double maxrep = REPAIR_RATE / (double)segments;

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
    maxrep *= (double)(ship.popn()) / (double)ship.max_crew();
    return (int)(0.005 * maxrep * shipcost(ship));
  }();

  if (cost <= ship.resource()) {
    use_resource(ship, cost);
    int drep = (int)maxrep;
    ship.damage() = std::max(0, (int)(ship.damage()) - drep);
  } else {
    /* use up all of the ships resources */
    int drep = (int)(maxrep * ((double)ship.resource() / (int)cost));
    use_resource(ship, ship.resource());
    ship.damage() = std::max(0, (int)(ship.damage()) - drep);
  }
}

void do_habitat(Ship& ship, EntityManager& entity_manager) {
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return;

  /* In v5.0+ Habitats make resources out of fuel */
  if (ship.on()) {
    double fuse = ship.fuel() *
                  ((double)ship.popn() / (double)ship.max_crew()) *
                  (1.0 - .01 * (double)ship.damage());
    auto add = (int)fuse / 20;
    if (ship.resource() + add > ship.max_resource())
      add = ship.max_resource() - ship.resource();
    fuse = 20.0 * (double)add;
    rcv_resource(ship, add);
    use_fuel(ship, fuse);

    for (auto nested_ship : ShipList(entity_manager, ship.ships())) {
      if (nested_ship->type() == ShipType::OTYPE_WPLANT)
        rcv_destruct(ship, do_weapon_plant(*nested_ship, entity_manager));
    }
  }

  auto add = round_rand((double)ship.popn() * race->birthrate);
  if (ship.popn() + add > max_crew(ship)) add = max_crew(ship) - ship.popn();
  rcv_popn(ship, add, race->mass);
}

void do_meta_infect(int who, starnum_t star, planetnum_t pnum, Planet& p,
                    EntityManager& entity_manager) {
  auto smap_handle = entity_manager.get_sectormap(star, pnum);
  if (!smap_handle.get()) return;
  auto& smap = *smap_handle;
  // TODO(jeffbailey): I'm pretty certain this memset is unnecessary, but this
  // is so far away from any other uses of Sectinfo that I'm having trouble
  // proving it.
  std::memset(Sectinfo, 0, sizeof(Sectinfo));
  auto x = int_rand(0, p.Maxx() - 1);
  auto y = int_rand(0, p.Maxy() - 1);
  auto owner = smap.get(x, y).get_owner();

  const auto* who_race = entity_manager.peek_race(who);
  if (!who_race) return;

  // Check if infection fails
  if (owner && owner == who) {
    return;  // Already owned by us
  }
  if (owner) {
    // Sector owned by someone else - check if we can take it
    const auto* owner_race = entity_manager.peek_race(owner);
    double fighters = owner_race ? owner_race->fighters : 1.0;
    double troops = smap.get(x, y).get_troops() * fighters / 50.0;
    if (int_rand(1, 100) <= 100.0 * (1.0 - std::exp(-troops))) {
      return;  // Failed to infect - defenders won
    }
  }

  // Infection succeeds
  p.info(who - 1).explored = 1;
  p.info(who - 1).numsectsowned += 1;
  smap.get(x, y).set_troops(0);
  smap.get(x, y).set_popn(who_race->number_sexes);
  smap.get(x, y).set_owner(who);
  smap.get(x, y).set_condition(smap.get(x, y).get_type());
  if (POD_TERRAFORM) {
    smap.get(x, y).set_condition(who_race->likesbest);
  }
}

int infect_planet(int who, starnum_t star, planetnum_t pnum,
                  EntityManager& entity_manager) {
  if (success(SPORE_SUCCESS_RATE)) {
    auto planet_handle = entity_manager.get_planet(star, pnum);
    if (planet_handle.get()) {
      do_meta_infect(who, star, pnum, *planet_handle, entity_manager);
      return 1;
    }
  }
  return 0;
}

void do_pod(Ship& ship, EntityManager& entity_manager) {
  if (!std::holds_alternative<PodData>(ship.special())) {
    return;
  }
  auto pod = std::get<PodData>(ship.special());

  switch (ship.whatorbits()) {
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = entity_manager.peek_star(ship.storbits());
      if (!star) return;

      if (pod.temperature < POD_THRESHOLD) {
        pod.temperature +=
            round_rand((double)star->temperature() / (double)segments);
        ship.special() = pod;
        return;
      }

      auto i = int_rand(0, star->numplanets() - 1);
      std::stringstream telegram_buf;
      telegram_buf << std::format("{} has warmed and exploded at {}\n",
                                  ship_to_string(ship), prin_ship_orbits(ship));
      if (infect_planet(ship.owner(), ship.storbits(), i, entity_manager)) {
        telegram_buf << std::format("\tmeta-colony established on {}.",
                                    star->get_planet_name(i));
      } else {
        telegram_buf << std::format("\tno spores have survived.");
      }
      push_telegram(ship.owner(), ship.governor(), telegram_buf.str());
      entity_manager.kill_ship(ship.owner(), ship);
      return;
    }

    case ScopeLevel::LEVEL_PLAN: {
      if (pod.decay < POD_DECAY) {
        pod.decay += round_rand(1.0 / (double)segments);
        ship.special() = pod;
        return;
      }

      std::string telegram =
          std::format("{} has decayed at {}\n", ship_to_string(ship),
                      prin_ship_orbits(ship));
      push_telegram(ship.owner(), ship.governor(), telegram);
      entity_manager.kill_ship(ship.owner(), ship);
      return;
    }

    default:
      // Doesn't apply at Universe or Ship
      return;
  }
}

void do_canister(Ship& ship, EntityManager& entity_manager, TurnStats& stats) {
  if (ship.whatorbits() != ScopeLevel::LEVEL_PLAN || landed(ship)) {
    return;
  }

  if (!std::holds_alternative<TimerData>(ship.special())) {
    return;
  }
  auto timer = std::get<TimerData>(ship.special());

  if (++timer.count < DISSIPATE) {
    ship.special() = timer;
    if (stats.Stinfo[ship.storbits()][ship.pnumorbits()].temp_add < -90)
      stats.Stinfo[ship.storbits()][ship.pnumorbits()].temp_add = -100;
    else
      stats.Stinfo[ship.storbits()][ship.pnumorbits()].temp_add -= 10;
  } else { /* timer expired; destroy canister */
    entity_manager.kill_ship(ship.owner(), ship);

    std::string telegram =
        std::format("Canister of dust previously covering {} has dissipated.\n",
                    prin_ship_orbits(ship));

    const auto* star = entity_manager.peek_star(ship.storbits());
    const auto* planet =
        entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
    if (star && planet) {
      for (auto race_handle : RaceList(entity_manager)) {
        const auto& race = race_handle.read();
        if (planet->info(race.Playernum - 1).numsectsowned)
          push_telegram(race.Playernum, star->governor(race.Playernum - 1),
                        telegram);
      }
    }
  }
}

void do_greenhouse(Ship& ship, EntityManager& entity_manager, TurnStats& stats) {
  if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN && !landed(ship)) {
    if (!std::holds_alternative<TimerData>(ship.special())) {
      return;
    }
    auto timer = std::get<TimerData>(ship.special());

    if (++timer.count < DISSIPATE) {
      ship.special() = timer;
      if (stats.Stinfo[ship.storbits()][ship.pnumorbits()].temp_add > 90)
        stats.Stinfo[ship.storbits()][ship.pnumorbits()].temp_add = 100;
      else
        stats.Stinfo[ship.storbits()][ship.pnumorbits()].temp_add += 10;
    } else { /* timer expired; destroy canister */
      entity_manager.kill_ship(ship.owner(), ship);
      std::string telegram = std::format(
          "Greenhouse gases at {} have dissipated.\n", prin_ship_orbits(ship));

      const auto* star = entity_manager.peek_star(ship.storbits());
      const auto* planet =
          entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
      if (star && planet) {
        for (auto race_handle : RaceList(entity_manager)) {
          const auto& race = race_handle.read();
          if (planet->info(race.Playernum - 1).numsectsowned)
            push_telegram(race.Playernum, star->governor(race.Playernum - 1),
                          telegram);
        }
      }
    }
  }
}

void do_mirror(Ship& ship, EntityManager& entity_manager, TurnStats& stats) {
  if (!std::holds_alternative<AimedAtData>(ship.special())) {
    return;
  }
  auto aimed_at = std::get<AimedAtData>(ship.special());

  switch (aimed_at.level) {
    case ScopeLevel::LEVEL_SHIP: { /* ship aimed at is a legal ship now */
      /* if in the same system */
      auto target_handle = entity_manager.get_ship(aimed_at.shipno);
      Ship* target = target_handle.get();
      if (!target) break;
      if ((ship.whatorbits() == ScopeLevel::LEVEL_STAR ||
           ship.whatorbits() == ScopeLevel::LEVEL_PLAN) &&
          (target->whatorbits() == ScopeLevel::LEVEL_STAR ||
           target->whatorbits() == ScopeLevel::LEVEL_PLAN) &&
          ship.storbits() == target->storbits() && target->alive()) {
        auto range = std::sqrt(
            Distsq(ship.xpos(), ship.ypos(), target->xpos(), target->ypos()));
        auto i = int_rand(0, round_rand((2. / ((double)(shipbody(*target)))) *
                                        (double)(aimed_at.intensity) /
                                        (range / PLORBITSIZE + 1.0)));
        std::stringstream telegram_buf;
        telegram_buf << std::format("{} aimed at {}\n", ship_to_string(ship),
                                    ship_to_string(*target));
        target->damage() += i;
        if (i) {
          telegram_buf << std::format("{}% damage done.\n", i);
        }
        if (target->damage() >= 100) {
          telegram_buf << std::format("{} DESTROYED!!!\n",
                                      ship_to_string(*target));
          entity_manager.kill_ship(ship.owner(), *target);
        }
        push_telegram(target->owner(), target->governor(), telegram_buf.str());
        push_telegram(ship.owner(), ship.governor(), telegram_buf.str());
      }
    } break;
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = entity_manager.peek_star(ship.storbits());
      const auto* planet =
          entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
      if (!star || !planet) break;

      double range = std::sqrt(Distsq(ship.xpos(), ship.ypos(),
                                      star->xpos() + planet->xpos(),
                                      star->ypos() + planet->ypos()));

      int i = range > PLORBITSIZE ? PLORBITSIZE * aimed_at.intensity / range
                                  : aimed_at.intensity;

      i = round_rand(.01 * (100.0 - (double)(ship.damage())) * (double)i);
      stats.Stinfo[ship.storbits()][aimed_at.pnum].temp_add += i;
    } break;
    case ScopeLevel::LEVEL_STAR:
      /* have to be in the same system as the star; otherwise
         it's not too fair.. */
      if (aimed_at.snum > 0 && aimed_at.snum < Sdata.numstars &&
          ship.whatorbits() > ScopeLevel::LEVEL_UNIV &&
          aimed_at.snum == ship.storbits()) {
        auto star_handle = entity_manager.get_star(aimed_at.snum);
        if (star_handle.get()) {
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<> dis(0, 1);
          star_handle->stability() += dis(gen);
        }
      }
      break;
    case ScopeLevel::LEVEL_UNIV:
      break;
  }
}

void do_god(Ship& ship, EntityManager& entity_manager) {
  /* gods have infinite power.... heh heh heh */
  const auto* race = entity_manager.peek_race(ship.owner());
  if (race && race->God) {
    ship.fuel() = max_fuel(ship);
    ship.destruct() = max_destruct(ship);
    ship.resource() = max_resource(ship);
  }
}

constexpr double ap_planet_factor(const Planet& p) {
  double x = p.Maxx() * p.Maxy();
  return (AP_FACTOR / (AP_FACTOR + x));
}

double crew_factor(const Ship& ship) {
  int maxcrew = Shipdata[ship.type()][ABIL_MAXCREW];

  if (!maxcrew) return 0.0;
  return ((double)ship.popn() / (double)maxcrew);
}

void do_ap(Ship& ship, EntityManager& entity_manager) {
  /* if landed on planet, change conditions to be like race */
  if (landed(ship) && ship.on()) {
    auto planet_handle =
        entity_manager.get_planet(ship.storbits(), ship.pnumorbits());
    const auto* race = entity_manager.peek_race(ship.owner());
    if (!planet_handle.get() || !race) return;
    auto& p = *planet_handle;

    if (ship.fuel() >= 3.0) {
      use_fuel(ship, 3.0);
      for (auto j = RTEMP + 1; j <= OTHER; j++) {
        auto d = round_rand(ap_planet_factor(p) * crew_factor(ship) *
                            (double)(race->conditions[j] -
                                     p.conditions(static_cast<Conditions>(j))));
        if (d) p.conditions(static_cast<Conditions>(j)) += d;
      }
    } else if (!ship.notified()) {
      ship.notified() = 1;
      ship.on() = 0;
      msg_OOF(ship);
    }
  }
}

void do_oap(Ship& ship) {
  /* "indimidate" the planet below, for enslavement purposes. */
  if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN)
    Stinfo[ship.storbits()][ship.pnumorbits()].intimidated = 1;
}
}  // namespace

void doship(Ship& ship, int update, EntityManager& entity_manager,
            TurnStats& stats) {
  /*ship is active */
  ship.active() = 1;

  if (!ship.owner()) ship.alive() = 0;

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

    if (!ship.popn() && max_crew(ship) && !ship.docked())
      ship.whatdest() = ScopeLevel::LEVEL_UNIV;

    // Check for supernova damage
    if (ship.whatorbits() != ScopeLevel::LEVEL_UNIV) {
      const auto* star = entity_manager.peek_star(ship.storbits());
      if (star && star->nova_stage() > 0) {
        /* damage ships from supernovae */
        /* Maarten: modified to take into account MOVES_PER_UPDATE */
        ship.damage() +=
            5L * star->nova_stage() / ((armor(ship) + 1) * segments);
        if (ship.damage() >= 100) {
          entity_manager.kill_ship(ship.owner(), ship);
          return;
        }
      }
    }

    if (ship.type() == ShipType::OTYPE_FACTORY && !ship.on()) {
      const auto* race = entity_manager.peek_race(ship.owner());
      if (race) ship.tech() = race->tech;
    }

    if (ship.active()) moveship(ship, update, 1, 0);

    ship.size() = ship_size(ship); /* for debugging */
    if (ship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      auto ship2 = entity_manager.get_ship(ship.destshipno());
      if (ship2.get() && ship2->owner() != ship.owner()) {
        ship2->owner() = ship.owner();
        ship2->governor() = ship.governor();
      }
      /* just making sure */
    } else if (ship.whatorbits() != ScopeLevel::LEVEL_UNIV &&
               (ship.popn() || ship.type() == ShipType::OTYPE_PROBE)) {
      /* Though I have often used TWCs for exploring, I don't think it is
       * right
       */
      /* to be able to map out worlds with this type of junk. Either a manned
       * ship, */
      /* or a probe, which is designed for this kind of work.  Maarten */
      stats.StarsInhab[ship.storbits()] = 1;
      auto star_handle = entity_manager.get_star(ship.storbits());
      if (star_handle.get()) {
        setbit(star_handle->inhabited(), ship.owner());
        setbit(star_handle->explored(), ship.owner());
      }
      if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
        auto planet_handle =
            entity_manager.get_planet(ship.storbits(), ship.pnumorbits());
        if (planet_handle.get()) {
          planet_handle->info(ship.owner() - 1).explored = 1;
        }
      }
    }

    /* add ships, popn to total count to add AP's */
    if (update) {
      stats.Power[ship.owner() - 1].ships_owned++;
      stats.Power[ship.owner() - 1].resource += ship.resource();
      stats.Power[ship.owner() - 1].fuel += ship.fuel();
      stats.Power[ship.owner() - 1].destruct += ship.destruct();
      stats.Power[ship.owner() - 1].popn += ship.popn();
      stats.Power[ship.owner() - 1].troops += ship.troops();
    }

    if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
      stats.Sdatanumships[ship.owner() - 1]++;
      stats.Sdatapopns[ship.owner()] += ship.popn();
    } else {
      stats.starnumships[ship.storbits()][ship.owner() - 1]++;
      /* add popn of ships to popn */
      stats.starpopns[ship.storbits()][ship.owner() - 1] += ship.popn();
      /* set inhabited for ship */
      /* only if manned or probe.  Maarten */
      if (ship.popn() || ship.type() == ShipType::OTYPE_PROBE) {
        stats.StarsInhab[ship.storbits()] = 1;
        auto star_handle = entity_manager.get_star(ship.storbits());
        if (star_handle.get()) {
          setbit(star_handle->inhabited(), ship.owner());
          setbit(star_handle->explored(), ship.owner());
        }
      }
    }

    if (ship.active()) {
      /* bombard the planet */
      if (can_bombard(ship) && ship.bombard() &&
          ship.whatorbits() == ScopeLevel::LEVEL_PLAN &&
          ship.whatdest() == ScopeLevel::LEVEL_PLAN &&
          ship.deststar() == ship.storbits() &&
          ship.destpnum() == ship.pnumorbits()) {
        /* ship bombards planet */
        stats.Stinfo[ship.storbits()][ship.pnumorbits()].inhab = 1;
      }

      /* repair ship by the amount of crew it has */
      /* industrial complexes can repair (robot ships
         and offline factories can't repair) */
      if (ship.damage() && repair(ship)) do_repair(ship, entity_manager);

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
            if (std::holds_alternative<MindData>(ship.special())) {
              auto mind = std::get<MindData>(ship.special());
              if (!mind.progenitor) {
                mind.progenitor = 1;
                ship.special() = mind;
              }
            } else {
              ship.special() = MindData{.progenitor = 1,
                                        .target = 0,
                                        .generation = 0,
                                        .busy = 0,
                                        .tampered = 0,
                                        .who_killed = 0};
            }
            do_VN(ship);
            break;
          case ShipType::STYPE_OAP:
            do_oap(ship);
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
  const auto* race = entity_manager.peek_race(ship.owner());
  double rmass = race ? race->mass : 1.0;  // Default mass if race not found

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
  if (!ship.alive() || !ship.owner()) return;
  if (!ship.on() || ship.docked()) return;

  /* check to see if it has arrived at it's destination */
  if (ship.whatdest() == ScopeLevel::LEVEL_PLAN &&
      ship.whatorbits() == ScopeLevel::LEVEL_PLAN &&
      ship.destpnum() == ship.pnumorbits()) {
    auto planet_handle =
        entity_manager.get_planet(ship.storbits(), ship.pnumorbits());
    if (!planet_handle.get()) return;
    auto& p = *planet_handle;

    // TODO(jeffbailey): Use std::ranges::find_if here once ShipList iterators
    // are made ranges-compatible (need default ctor, post-increment, etc.)
    /* check to see if PDNs are present */
    bool found_pdn = false;
    for (auto pdn_ship : ShipList(entity_manager, p.ships())) {
      if (!pdn_ship->alive() || pdn_ship->type() != ShipType::OTYPE_PLANDEF) {
        continue;
      }
      /* attack the PDN instead */
      ship.whatdest() =
          ScopeLevel::LEVEL_SHIP; /* move missile to PDN for attack */
      ship.xpos() = pdn_ship->xpos();
      ship.ypos() = pdn_ship->ypos();
      ship.destshipno() = pdn_ship->number();
      found_pdn = true;
      break;
    }
    if (!found_pdn) {
      auto [bombx, bomby] = [&p, &ship] -> std::tuple<int, int> {
        if (std::holds_alternative<ImpactData>(ship.special())) {
          auto impact = std::get<ImpactData>(ship.special());
          if (impact.scatter) {
            auto bombx = int_rand(1, (int)p.Maxx()) - 1;
            auto bomby = int_rand(1, (int)p.Maxy()) - 1;
            return {bombx, bomby};
          } else {
            auto bombx = impact.x % p.Maxx();
            auto bomby = impact.y % p.Maxy();
            return {bombx, bomby};
          }
        } else {
          // Default to random if no impact data
          auto bombx = int_rand(1, (int)p.Maxx()) - 1;
          auto bomby = int_rand(1, (int)p.Maxy()) - 1;
          return {bombx, bomby};
        }
      }();

      // TODO(jeffbailey): This doesn't actually notify anyone and should.
      std::string bombdropmsg = std::format(
          "{} dropped on sector {},{} at planet {}.\n", ship_to_string(ship),
          bombx, bomby, prin_ship_orbits(ship));

      auto smap_handle =
          entity_manager.get_sectormap(ship.storbits(), ship.pnumorbits());
      if (!smap_handle.get()) return;
      auto& smap = *smap_handle;
      char long_buf[1024], short_buf[256];
      auto numdest =
          shoot_ship_to_planet(ship, p, (int)ship.destruct(), bombx, bomby,
                               smap, 0, GTYPE_HEAVY, long_buf, short_buf);
      push_telegram(ship.owner(), ship.governor(), long_buf);
      entity_manager.kill_ship(ship.owner(), ship);
      std::string sectors_destroyed_msg =
          std::format("{} dropped on {}.\n\t{} sectors destroyed.\n",
                      ship_to_string(ship), prin_ship_orbits(ship), numdest);
      const auto* star = entity_manager.peek_star(ship.storbits());
      for (auto race_handle : RaceList(entity_manager)) {
        const auto& race = race_handle.read();
        if (p.info(race.Playernum - 1).numsectsowned &&
            race.Playernum != ship.owner()) {
          push_telegram(race.Playernum,
                        star ? star->governor(race.Playernum - 1) : 0,
                        sectors_destroyed_msg);
        }
      }
      if (numdest) {
        std::string dropmsg =
            std::format("{} dropped on {}.\n", ship_to_string(ship),
                        prin_ship_orbits(ship));
        post(dropmsg, NewsType::COMBAT);
      }
    }
  } else if (ship.whatdest() == ScopeLevel::LEVEL_SHIP) {
    auto sh2 = ship.destshipno();
    auto target_handle = entity_manager.get_ship(sh2);
    Ship* target = target_handle.get();
    if (!target) return;

    auto dist = std::sqrt(
        Distsq(ship.xpos(), ship.ypos(), target->xpos(), target->ypos()));
    if (dist <= ((double)ship.speed() * STRIKE_DISTANCE_FACTOR *
                 (100.0 - (double)ship.damage()) / 100.0)) {
      /* do the attack */
      auto s2sresult =
          shoot_ship_to_ship(ship, *target, (int)ship.destruct(), 0);
      auto const& [damage, short_buf, long_buf] = *s2sresult;
      push_telegram(ship.owner(), ship.governor(), long_buf);
      push_telegram(target->owner(), target->governor(), long_buf);
      entity_manager.kill_ship(ship.owner(), ship);
      post(short_buf, NewsType::COMBAT);
    }
  }
}

void domine(Ship& ship, int detonate, EntityManager& entity_manager) {
  if (ship.type() != ShipType::STYPE_MINE || !ship.alive() || !ship.owner()) {
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
      const auto* star = entity_manager.peek_star(ship.storbits());
      return star ? star->ships() : 0;
    } else {  // ScopeLevel::LEVEL_PLAN
      const auto planet = getplanet(ship.storbits(), ship.pnumorbits());
      return planet.ships();
    }
  }();

  // traverse the list, look for ships that are closer than the trigger
  // radius.
  bool rad = false;
  if (!detonate) {
    const auto* race = entity_manager.peek_race(ship.owner());
    if (!race) return;

    const ShipList kShiplist(entity_manager, sh);
    for (const Ship* s_ptr : kShiplist) {
      const Ship& s = *s_ptr;
      double xd = s.xpos() - ship.xpos();
      double yd = s.ypos() - ship.ypos();
      double range = std::sqrt(xd * xd + yd * yd);
      if (!isset(race->allied, s.owner()) && (s.owner() != ship.owner()) &&
          std::holds_alternative<TriggerData>(ship.special()) &&
          ((int)range <= std::get<TriggerData>(ship.special()).radius)) {
        rad = true;
        break;
      }
    }
  } else {
    rad = true;
  }

  if (!rad) {
    return;
  }

  std::string postmsg = std::format(
      "{} detonated at {}\n", ship_to_string(ship), prin_ship_orbits(ship));
  post(postmsg, NewsType::COMBAT);
  notify_star(ship.owner(), ship.governor(), ship.storbits(), postmsg);
  ShipList shiplist(entity_manager, sh);
  for (auto ship_handle : shiplist) {
    Ship& s = *ship_handle;
    if (sh != ship.number() && s.alive() &&
        (s.type() != ShipType::OTYPE_CANIST) &&
        (s.type() != ShipType::OTYPE_GREEN)) {
      auto s2sresult =
          shoot_ship_to_ship(ship, s, (int)(ship.destruct()), 0, false);
      if (s2sresult) {
        auto const& [damage, short_buf, long_buf] = *s2sresult;
        post(short_buf, NewsType::COMBAT);
        warn(s.owner(), s.governor(), long_buf);
      }
    }
  }

  /* if the mine is in orbit around a planet, nuke the planet too! */
  if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    /* pick a random sector to nuke */
    auto planet = getplanet(ship.storbits(), ship.pnumorbits());

    auto [x, y] = [&ship, &planet]() -> std::pair<int, int> {
      if (landed(ship)) {
        return {ship.land_x(), ship.land_y()};
      } else {
        int x = int_rand(0, (int)planet.Maxx() - 1);
        int y = int_rand(0, (int)planet.Maxy() - 1);
        return {x, y};
      }
    }();

    auto smap = getsmap(planet);
    char long_buf[1024], short_buf[256];
    auto numdest =
        shoot_ship_to_planet(ship, planet, (int)(ship.destruct()), x, y, smap,
                             0, GTYPE_LIGHT, long_buf, short_buf);
    putsmap(smap, planet);

    const auto* star = entity_manager.peek_star(ship.storbits());
    if (!star) return;
    putplanet(planet, *star, (int)ship.pnumorbits());

    std::stringstream telegram;
    telegram << postmsg;
    if (numdest > 0) {
      telegram << std::format(" - {} sectors destroyed.", numdest);
    }
    telegram << "\n";
    for (auto race_handle : RaceList(entity_manager)) {
      const auto& race = race_handle.read();
      if (Nuked[race.Playernum - 1]) {
        warn(race.Playernum, star->governor(race.Playernum - 1),
             telegram.str());
      }
    }
    notify(ship.owner(), ship.governor(), telegram.str());
  }

  entity_manager.kill_ship(ship.owner(), ship);
}

void doabm(Ship& ship, EntityManager& entity_manager) {
  if (!ship.alive() || !ship.owner()) return;
  if (!ship.on() || !ship.retaliate() || !ship.destruct()) return;

  if (landed(ship)) {
    const auto* planet =
        entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
    if (!planet) return;

    const auto* owner_race = entity_manager.peek_race(ship.owner());
    if (!owner_race) return;

    /* check to see if missiles/mines are present */
    for (auto target_handle : ShipList(entity_manager, planet->ships())) {
      if (!ship.destruct()) break;  // Exit if out of destruct

      Ship& target = *target_handle;
      if (!target.alive()) continue;
      if (target.type() != ShipType::STYPE_MISSILE &&
          target.type() != ShipType::STYPE_MINE)
        continue;
      if (target.owner() == ship.owner()) continue;

      // Check alliance status
      const auto* target_race = entity_manager.peek_race(target.owner());
      if (target_race && isset(owner_race->allied, target.owner()) &&
          isset(target_race->allied, ship.owner())) {
        /* mutually allied missiles don't get shot up */
        continue;
      }

      /* attack the missile/mine */
      auto numdest = retal_strength(ship);
      numdest = MIN(numdest, ship.destruct());
      numdest = MIN(numdest, ship.retaliate());
      ship.destruct() -= numdest;
      auto const& s2sresult = shoot_ship_to_ship(ship, target, numdest, 0);
      auto [damage, short_buf, long_buf] = *s2sresult;
      push_telegram(ship.owner(), ship.governor(), long_buf);
      push_telegram(target.owner(), target.governor(), long_buf);
      post(short_buf, NewsType::COMBAT);
    }
  }
}

int do_weapon_plant(Ship& ship, EntityManager& entity_manager) {
  const auto* race = entity_manager.peek_race(ship.owner());
  double tech = race ? race->tech : 0.0;
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
