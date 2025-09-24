// SPDX-License-Identifier: Apache-2.0

module;

/* doship -- do one ship turn. */

import std;

module gblib;

namespace {
void do_repair(Ship &ship) {
  double maxrep = REPAIR_RATE / (double)segments;

  /* stations repair for free, and ships docked with them */
  int cost = [&ship, &maxrep]() {
    if (Shipdata[ship.type][ABIL_REPAIR] ||
        (ship.docked && ship.whatdest == ScopeLevel::LEVEL_SHIP &&
         ships[ship.destshipno]->type == ShipType::STYPE_STATION) ||
        (ship.docked && ship.whatorbits == ScopeLevel::LEVEL_SHIP &&
         ships[ship.destshipno]->type == ShipType::STYPE_STATION)) {
      return 0;
    } else {
      maxrep *= (double)(ship.popn) / (double)ship.max_crew;
      return (int)(0.005 * maxrep * shipcost(ship));
    }
  }();

  if (cost <= ship.resource) {
    use_resource(ship, cost);
    int drep = (int)maxrep;
    ship.damage = std::max(0, (int)(ship.damage) - drep);
  } else {
    /* use up all of the ships resources */
    int drep = (int)(maxrep * ((double)ship.resource / (int)cost));
    use_resource(ship, ship.resource);
    ship.damage = std::max(0, (int)(ship.damage) - drep);
  }
}

void do_habitat(Ship &ship) {
  /* In v5.0+ Habitats make resources out of fuel */
  if (ship.on) {
    double fuse = ship.fuel * ((double)ship.popn / (double)ship.max_crew) *
                  (1.0 - .01 * (double)ship.damage);
    auto add = (int)fuse / 20;
    if (ship.resource + add > ship.max_resource)
      add = ship.max_resource - ship.resource;
    fuse = 20.0 * (double)add;
    rcv_resource(ship, add);
    use_fuel(ship, fuse);

    auto sh = ship.ships;
    while (sh) {
      if (ships[sh]->type == ShipType::OTYPE_WPLANT)
        rcv_destruct(ship, do_weapon_plant(*ships[sh]));
      sh = ships[sh]->nextship;
    }
  }

  auto add = round_rand((double)ship.popn * races[ship.owner - 1].birthrate);
  if (ship.popn + add > max_crew(ship)) add = max_crew(ship) - ship.popn;
  rcv_popn(ship, add, races[ship.owner - 1].mass);
}

void do_meta_infect(int who, Planet &p) {
  auto smap = getsmap(p);
  // TODO(jeffbailey): I'm pretty certain this memset is unnecessary, but this
  // is so far away from any other uses of Sectinfo that I'm having trouble
  // proving it.
  std::memset(Sectinfo, 0, sizeof(Sectinfo));
  auto x = int_rand(0, p.Maxx - 1);
  auto y = int_rand(0, p.Maxy - 1);
  auto owner = smap.get(x, y).owner;
  if (!owner ||
      (who != owner &&
       (double)int_rand(1, 100) >
           100.0 * (1.0 -
                    std::exp(-((double)(smap.get(x, y).troops *
                                        races[owner - 1].fighters / 50.0)))))) {
    p.info[who - 1].explored = 1;
    p.info[who - 1].numsectsowned += 1;
    smap.get(x, y).troops = 0;
    smap.get(x, y).popn = races[who - 1].number_sexes;
    smap.get(x, y).owner = who;
    smap.get(x, y).condition = smap.get(x, y).type;
    if (POD_TERRAFORM) {
      smap.get(x, y).condition = races[who - 1].likesbest;
    }
    putsmap(smap, p);
  }
}

int infect_planet(int who, int star, int p) {
  if (success(SPORE_SUCCESS_RATE)) {
    do_meta_infect(who, *planets[star][p]);
    return 1;
  }
  return 0;
}

void do_pod(Ship &ship) {
  if (!std::holds_alternative<PodData>(ship.special)) {
    return;
  }
  auto pod = std::get<PodData>(ship.special);
  
  switch (ship.whatorbits) {
    case ScopeLevel::LEVEL_STAR: {
      if (pod.temperature < POD_THRESHOLD) {
        pod.temperature += round_rand(
            (double)stars[ship.storbits].temperature() / (double)segments);
        ship.special = pod;
        return;
      }

      auto i = int_rand(0, stars[ship.storbits].numplanets() - 1);
      std::stringstream telegram_buf;
      telegram_buf << std::format("{} has warmed and exploded at {}\n",
                                  ship_to_string(ship), prin_ship_orbits(ship));
      if (infect_planet(ship.owner, ship.storbits, i)) {
        telegram_buf << std::format("\tmeta-colony established on {}.",
                                    stars[ship.storbits].get_planet_name(i));
      } else {
        telegram_buf << std::format("\tno spores have survived.");
      }
      push_telegram(ship.owner, ship.governor, telegram_buf.str());
      kill_ship(ship.owner, &ship);
      return;
    }

    case ScopeLevel::LEVEL_PLAN: {
      if (pod.decay < POD_DECAY) {
        pod.decay += round_rand(1.0 / (double)segments);
        ship.special = pod;
        return;
      }

      std::string telegram =
          std::format("{} has decayed at {}\n", ship_to_string(ship),
                      prin_ship_orbits(ship));
      push_telegram(ship.owner, ship.governor, telegram);
      kill_ship(ship.owner, &ship);
      return;
    }

    default:
      // Doesn't apply at Universe or Ship
      return;
  }
}

void do_canister(Ship &ship) {
  if (ship.whatorbits != ScopeLevel::LEVEL_PLAN || landed(ship)) {
    return;
  }
  
  if (!std::holds_alternative<TimerData>(ship.special)) {
    return;
  }
  auto timer = std::get<TimerData>(ship.special);
  
  if (++timer.count < DISSIPATE) {
    ship.special = timer;
    if (Stinfo[ship.storbits][ship.pnumorbits].temp_add < -90)
      Stinfo[ship.storbits][ship.pnumorbits].temp_add = -100;
    else
      Stinfo[ship.storbits][ship.pnumorbits].temp_add -= 10;
  } else { /* timer expired; destroy canister */
    kill_ship(ship.owner, &ship);

    std::string telegram =
        std::format("Canister of dust previously covering {} has dissipated.\n",
                    prin_ship_orbits(ship));

    for (int j = 1; j <= Num_races; j++)
      if (planets[ship.storbits][ship.pnumorbits]->info[j - 1].numsectsowned)
        push_telegram(j, stars[ship.storbits].governor(j - 1), telegram);
  }
}

void do_greenhouse(Ship &ship) {
  if (ship.whatorbits == ScopeLevel::LEVEL_PLAN && !landed(ship)) {
    if (++ship.special.timer.count < DISSIPATE) {
      if (Stinfo[ship.storbits][ship.pnumorbits].temp_add > 90)
        Stinfo[ship.storbits][ship.pnumorbits].temp_add = 100;
      else
        Stinfo[ship.storbits][ship.pnumorbits].temp_add += 10;
    } else { /* timer expired; destroy canister */
      int j = 0;

      kill_ship(ship.owner, &ship);
      std::string telegram = std::format(
          "Greenhouse gases at {} have dissipated.\n", prin_ship_orbits(ship));
      for (j = 1; j <= Num_races; j++)
        if (planets[ship.storbits][ship.pnumorbits]->info[j - 1].numsectsowned)
          push_telegram(j, stars[ship.storbits].governor(j - 1), telegram);
    }
  }
}

void do_mirror(Ship &ship) {
  switch (ship.special.aimed_at.level) {
    case ScopeLevel::LEVEL_SHIP: /* ship aimed at is a legal ship now */
      /* if in the same system */
      if ((ship.whatorbits == ScopeLevel::LEVEL_STAR ||
           ship.whatorbits == ScopeLevel::LEVEL_PLAN) &&
          (ships[ship.special.aimed_at.shipno] != nullptr) &&
          (ships[ship.special.aimed_at.shipno]->whatorbits ==
               ScopeLevel::LEVEL_STAR ||
           ships[ship.special.aimed_at.shipno]->whatorbits ==
               ScopeLevel::LEVEL_PLAN) &&
          ship.storbits == ships[ship.special.aimed_at.shipno]->storbits &&
          ships[ship.special.aimed_at.shipno]->alive) {
        auto s = ships[ship.special.aimed_at.shipno];
        auto range = std::sqrt(Distsq(ship.xpos, ship.ypos, s->xpos, s->ypos));
        auto i =
            int_rand(0, round_rand((2. / ((double)(shipbody(*s)))) *
                                   (double)(ship.special.aimed_at.intensity) /
                                   (range / PLORBITSIZE + 1.0)));
        std::stringstream telegram_buf;
        telegram_buf << std::format("{} aimed at {}\n", ship_to_string(ship),
                                    ship_to_string(*s));
        s->damage += i;
        if (i) {
          telegram_buf << std::format("{}% damage done.\n", i);
        }
        if (s->damage >= 100) {
          telegram_buf << std::format("{} DESTROYED!!!\n", ship_to_string(*s));
          kill_ship(ship.owner, s);
        }
        push_telegram(s->owner, s->governor, telegram_buf.str());
        push_telegram(ship.owner, ship.governor, telegram_buf.str());
      }
      break;
    case ScopeLevel::LEVEL_PLAN: {
      double range =
          std::sqrt(Distsq(ship.xpos, ship.ypos,
                           stars[ship.storbits].xpos() +
                               planets[ship.storbits][ship.pnumorbits]->xpos,
                           stars[ship.storbits].ypos() +
                               planets[ship.storbits][ship.pnumorbits]->ypos));

      int i = range > PLORBITSIZE
                  ? PLORBITSIZE * ship.special.aimed_at.intensity / range
                  : ship.special.aimed_at.intensity;

      i = round_rand(.01 * (100.0 - (double)(ship.damage)) * (double)i);
      Stinfo[ship.storbits][ship.special.aimed_at.pnum].temp_add += i;
    } break;
    case ScopeLevel::LEVEL_STAR:
      /* have to be in the same system as the star; otherwise
         it's not too fair.. */
      if (ship.special.aimed_at.snum > 0 &&
          ship.special.aimed_at.snum < Sdata.numstars &&
          ship.whatorbits > ScopeLevel::LEVEL_UNIV &&
          ship.special.aimed_at.snum == ship.storbits) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1);
        stars[ship.special.aimed_at.snum].stability() += dis(gen);
      }
      break;
    case ScopeLevel::LEVEL_UNIV:
      break;
  }
}

void do_god(Ship &ship) {
  /* gods have infinite power.... heh heh heh */
  if (races[ship.owner - 1].God) {
    ship.fuel = max_fuel(ship);
    ship.destruct = max_destruct(ship);
    ship.resource = max_resource(ship);
  }
}

constexpr double ap_planet_factor(const Planet &p) {
  double x = p.Maxx * p.Maxy;
  return (AP_FACTOR / (AP_FACTOR + x));
}

double crew_factor(const Ship &ship) {
  int maxcrew = Shipdata[ship.type][ABIL_MAXCREW];

  if (!maxcrew) return 0.0;
  return ((double)ship.popn / (double)maxcrew);
}

void do_ap(Ship &ship) {
  /* if landed on planet, change conditions to be like race */
  if (landed(ship) && ship.on) {
    // TODO(jeffbailey): Not obvious here how the modified planet is saved
    // to disk
    auto &p = planets[ship.storbits][ship.pnumorbits];
    auto &race = races[ship.owner - 1];
    if (ship.fuel >= 3.0) {
      use_fuel(ship, 3.0);
      for (auto j = RTEMP + 1; j <= OTHER; j++) {
        auto d = round_rand(ap_planet_factor(*p) * crew_factor(ship) *
                            (double)(race.conditions[j] - p->conditions[j]));
        if (d) p->conditions[j] += d;
      }
    } else if (!ship.notified) {
      ship.notified = 1;
      ship.on = 0;
      msg_OOF(ship);
    }
  }
}

void do_oap(Ship &ship) {
  /* "indimidate" the planet below, for enslavement purposes. */
  if (ship.whatorbits == ScopeLevel::LEVEL_PLAN)
    Stinfo[ship.storbits][ship.pnumorbits].intimidated = 1;
}
}  // namespace

void doship(Ship &ship, int update) {
  /*ship is active */
  ship.active = 1;

  if (!ship.owner) ship.alive = 0;

  if (ship.alive) {
    /* repair radiation */
    if (ship.rad) {
      ship.active = 1;
      /* irradiated ships are immobile.. */
      /* kill off some people */
      /* check to see if ship is active */
      if (success(ship.rad)) ship.active = 0;
      if (update) {
        ship.popn = round_rand(ship.popn * .80);
        ship.troops = round_rand(ship.troops * .80);
        if (ship.rad >= (int)REPAIR_RATE)
          ship.rad -= int_rand(0, (int)REPAIR_RATE);
        else
          ship.rad -= int_rand(0, (int)ship.rad);
      }
    } else
      ship.active = 1;

    if (!ship.popn && max_crew(ship) && !ship.docked)
      ship.whatdest = ScopeLevel::LEVEL_UNIV;

    if (ship.whatorbits != ScopeLevel::LEVEL_UNIV &&
        stars[ship.storbits].nova_stage() > 0) {
      /* damage ships from supernovae */
      /* Maarten: modified to take into account MOVES_PER_UPDATE */
      ship.damage += 5L * stars[ship.storbits].nova_stage() /
                     ((armor(ship) + 1) * segments);
      if (ship.damage >= 100) {
        kill_ship(ship.owner, &ship);
        return;
      }
    }

    if (ship.type == ShipType::OTYPE_FACTORY && !ship.on) {
      auto &race = races[ship.owner - 1];
      ship.tech = race.tech;
    }

    if (ship.active) moveship(ship, update, 1, 0);

    ship.size = ship_size(ship); /* for debugging */

    if (ship.whatorbits == ScopeLevel::LEVEL_SHIP) {
      auto ship2 = getship(ship.destshipno);
      if (ship2->owner != ship.owner) {
        ship2->owner = ship.owner;
        ship2->governor = ship.governor;
        putship(*ship2);
      }
      /* just making sure */
    } else if (ship.whatorbits != ScopeLevel::LEVEL_UNIV &&
               (ship.popn || ship.type == ShipType::OTYPE_PROBE)) {
      /* Though I have often used TWCs for exploring, I don't think it is
       * right
       */
      /* to be able to map out worlds with this type of junk. Either a manned
       * ship, */
      /* or a probe, which is designed for this kind of work.  Maarten */
      StarsInhab[ship.storbits] = 1;
      setbit(stars[ship.storbits].inhabited(), ship.owner);
      setbit(stars[ship.storbits].explored(), ship.owner);
      if (ship.whatorbits == ScopeLevel::LEVEL_PLAN) {
        planets[ship.storbits][ship.pnumorbits]->info[ship.owner - 1].explored =
            1;
      }
    }

    /* add ships, popn to total count to add AP's */
    if (update) {
      Power[ship.owner - 1].ships_owned++;
      Power[ship.owner - 1].resource += ship.resource;
      Power[ship.owner - 1].fuel += ship.fuel;
      Power[ship.owner - 1].destruct += ship.destruct;
      Power[ship.owner - 1].popn += ship.popn;
      Power[ship.owner - 1].troops += ship.troops;
    }

    if (ship.whatorbits == ScopeLevel::LEVEL_UNIV) {
      Sdatanumships[ship.owner - 1]++;
      Sdatapopns[ship.owner] += ship.popn;
    } else {
      starnumships[ship.storbits][ship.owner - 1]++;
      /* add popn of ships to popn */
      starpopns[ship.storbits][ship.owner - 1] += ship.popn;
      /* set inhabited for ship */
      /* only if manned or probe.  Maarten */
      if (ship.popn || ship.type == ShipType::OTYPE_PROBE) {
        StarsInhab[ship.storbits] = 1;
        setbit(stars[ship.storbits].inhabited(), ship.owner);
        setbit(stars[ship.storbits].explored(), ship.owner);
      }
    }

    if (ship.active) {
      /* bombard the planet */
      if (can_bombard(ship) && ship.bombard &&
          ship.whatorbits == ScopeLevel::LEVEL_PLAN &&
          ship.whatdest == ScopeLevel::LEVEL_PLAN &&
          ship.deststar == ship.storbits && ship.destpnum == ship.pnumorbits) {
        /* ship bombards planet */
        Stinfo[ship.storbits][ship.pnumorbits].inhab = 1;
      }

      /* repair ship by the amount of crew it has */
      /* industrial complexes can repair (robot ships
         and offline factories can't repair) */
      if (ship.damage && repair(ship)) do_repair(ship);

      if (update) switch (ship.type) { /* do this stuff during updates only*/
          case ShipType::OTYPE_CANIST:
            do_canister(ship);
            break;
          case ShipType::OTYPE_GREEN:
            do_greenhouse(ship);
            break;
          case ShipType::STYPE_MIRROR:
            do_mirror(ship);
            break;
          case ShipType::STYPE_GOD:
            do_god(ship);
            break;
          case ShipType::OTYPE_AP:
            do_ap(ship);
            break;
          case ShipType::OTYPE_VN: /* Von Neumann machine */
          case ShipType::OTYPE_BERS:
            if (!ship.special.mind.progenitor) ship.special.mind.progenitor = 1;
            do_VN(ship);
            break;
          case ShipType::STYPE_OAP:
            do_oap(ship);
            break;
          case ShipType::STYPE_HABITAT:
            do_habitat(ship);
            break;
          default:
            break;
        }
      if (ship.type == ShipType::STYPE_POD) do_pod(ship);
    }
  }
}

void domass(Ship &ship) {
  auto rmass = races[ship.owner - 1].mass;

  auto sh = ship.ships;
  ship.mass = 0.0;
  ship.hanger = 0;
  while (sh) {
    domass(*ships[sh]); /* recursive call */
    ship.mass += ships[sh]->mass;
    ship.hanger += ships[sh]->size;
    sh = ships[sh]->nextship;
  }
  ship.mass += getmass(ship);
  ship.mass += (double)(ship.popn + ship.troops) * rmass;
  ship.mass += (double)ship.destruct * MASS_DESTRUCT;
  ship.mass += ship.fuel * MASS_FUEL;
  ship.mass += (double)ship.resource * MASS_RESOURCE;
}

void doown(Ship &ship) {
  auto sh = ship.ships;
  while (sh) {
    doown(*ships[sh]); /* recursive call */
    ships[sh]->owner = ship.owner;
    ships[sh]->governor = ship.governor;
    sh = ships[sh]->nextship;
  }
}

void domissile(Ship &ship) {
  if (!ship.alive || !ship.owner) return;
  if (!ship.on || ship.docked) return;

  /* check to see if it has arrived at it's destination */
  if (ship.whatdest == ScopeLevel::LEVEL_PLAN &&
      ship.whatorbits == ScopeLevel::LEVEL_PLAN &&
      ship.destpnum == ship.pnumorbits) {
    auto &p = planets[ship.storbits][ship.pnumorbits];
    /* check to see if PDNs are present */
    auto pdn = 0;
    auto sh2 = p->ships;
    while (sh2 && !pdn) {
      if (ships[sh2]->alive && ships[sh2]->type == ShipType::OTYPE_PLANDEF) {
        /* attack the PDN instead */
        ship.whatdest =
            ScopeLevel::LEVEL_SHIP; /* move missile to PDN for attack */
        ship.xpos = ships[sh2]->xpos;
        ship.ypos = ships[sh2]->ypos;
        ship.destshipno = sh2;
        pdn = sh2;
      }
      sh2 = ships[sh2]->nextship;
    }
    if (!pdn) {
      auto [bombx, bomby] = [&p, &ship] -> std::tuple<int, int> {
        if (std::holds_alternative<ImpactData>(ship.special)) {
          auto impact = std::get<ImpactData>(ship.special);
          if (impact.scatter) {
            auto bombx = int_rand(1, (int)p->Maxx) - 1;
            auto bomby = int_rand(1, (int)p->Maxy) - 1;
            return {bombx, bomby};
          } else {
            auto bombx = impact.x % p->Maxx;
            auto bomby = impact.y % p->Maxy;
            return {bombx, bomby};
          }
        } else {
          // Default to random if no impact data
          auto bombx = int_rand(1, (int)p->Maxx) - 1;
          auto bomby = int_rand(1, (int)p->Maxy) - 1;
          return {bombx, bomby};
        }
      }();

      // TODO(jeffbailey): This doesn't actually notify anyone and should.
      std::string bombdropmsg = std::format(
          "{} dropped on sector {},{} at planet {}.\n", ship_to_string(ship),
          bombx, bomby, prin_ship_orbits(ship));

      auto smap = getsmap(*p);
      char long_buf[1024], short_buf[256];
      auto numdest =
          shoot_ship_to_planet(ship, *p, (int)ship.destruct, bombx, bomby, smap,
                               0, GTYPE_HEAVY, long_buf, short_buf);
      putsmap(smap, *p);
      push_telegram(ship.owner, ship.governor, long_buf);
      kill_ship(ship.owner, &ship);
      std::string sectors_destroyed_msg =
          std::format("{} dropped on {}.\n\t{} sectors destroyed.\n",
                      ship_to_string(ship), prin_ship_orbits(ship), numdest);
      for (auto i = 1; i <= Num_races; i++) {
        if (p->info[i - 1].numsectsowned && i != ship.owner) {
          push_telegram(i, stars[ship.storbits].governor(i - 1),
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
  } else if (ship.whatdest == ScopeLevel::LEVEL_SHIP) {
    auto sh2 = ship.destshipno;
    auto dist = std::sqrt(
        Distsq(ship.xpos, ship.ypos, ships[sh2]->xpos, ships[sh2]->ypos));
    if (dist <= ((double)ship.speed * STRIKE_DISTANCE_FACTOR *
                 (100.0 - (double)ship.damage) / 100.0)) {
      /* do the attack */
      auto s2sresult =
          shoot_ship_to_ship(ship, *ships[sh2], (int)ship.destruct, 0);
      auto const &[damage, short_buf, long_buf] = *s2sresult;
      push_telegram(ship.owner, ship.governor, long_buf);
      push_telegram(ships[sh2]->owner, ships[sh2]->governor, long_buf);
      kill_ship(ship.owner, &ship);
      post(short_buf, NewsType::COMBAT);
    }
  }
}

void domine(Ship &ship, int detonate) {
  if (ship.type != ShipType::STYPE_MINE || !ship.alive || !ship.owner) {
    return;
  }

  /* check around and see if we should explode. */
  if (!ship.on && !detonate) {
    return;
  }

  if (ship.whatorbits == ScopeLevel::LEVEL_UNIV ||
      ship.whatorbits == ScopeLevel::LEVEL_SHIP)
    return;

  auto sh = [&ship] -> shipnum_t {
    if (ship.whatorbits == ScopeLevel::LEVEL_STAR) {
      return stars[ship.storbits].ships();
    } else {  // ScopeLevel::LEVEL_PLAN
      const auto planet = getplanet(ship.storbits, ship.pnumorbits);
      return planet.ships;
    }
  }();

  // traverse the list, look for ships that are closer than the trigger
  // radius.
  bool rad = false;
  if (!detonate) {
    auto &r = races[ship.owner - 1];
    Shiplist shiplist(sh);
    for (auto s : shiplist) {
      double xd = s.xpos - ship.xpos;
      double yd = s.ypos - ship.ypos;
      double range = std::sqrt(xd * xd + yd * yd);
      if (!isset(r.allied, s.owner) && (s.owner != ship.owner) &&
          ((int)range <= ship.special.trigger.radius)) {
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
  notify_star(ship.owner, ship.governor, ship.storbits, postmsg);
  Shiplist shiplist(sh);
  for (auto s : shiplist) {
    if (sh != ship.number && s.alive && (s.type != ShipType::OTYPE_CANIST) &&
        (s.type != ShipType::OTYPE_GREEN)) {
      auto s2sresult =
          shoot_ship_to_ship(ship, s, (int)(ship.destruct), 0, false);
      if (s2sresult) {
        auto const &[damage, short_buf, long_buf] = *s2sresult;
        post(short_buf, NewsType::COMBAT);
        warn(s.owner, s.governor, long_buf);
        putship(s);
      }
    }
  }

  /* if the mine is in orbit around a planet, nuke the planet too! */
  if (ship.whatorbits == ScopeLevel::LEVEL_PLAN) {
    /* pick a random sector to nuke */
    auto planet = getplanet(ship.storbits, ship.pnumorbits);

    auto [x, y] = [&ship, &planet]() -> std::pair<int, int> {
      if (landed(ship)) {
        return {ship.land_x, ship.land_y};
      } else {
        int x = int_rand(0, (int)planet.Maxx - 1);
        int y = int_rand(0, (int)planet.Maxy - 1);
        return {x, y};
      }
    }();

    auto smap = getsmap(planet);
    char long_buf[1024], short_buf[256];
    auto numdest =
        shoot_ship_to_planet(ship, planet, (int)(ship.destruct), x, y, smap, 0,
                             GTYPE_LIGHT, long_buf, short_buf);
    putsmap(smap, planet);
    putplanet(planet, stars[ship.storbits], (int)ship.pnumorbits);

    std::stringstream telegram;
    telegram << postmsg;
    if (numdest > 0) {
      telegram << std::format(" - {} sectors destroyed.", numdest);
    }
    telegram << "\n";
    for (auto i = 1; i <= Num_races; i++) {
      if (Nuked[i - 1]) {
        warn(i, stars[ship.storbits].governor(i - 1), telegram.str());
      }
    }
    notify(ship.owner, ship.governor, telegram.str());
  }

  kill_ship(ship.owner, &ship);
  putship(ship);
}

void doabm(Ship &ship) {
  if (!ship.alive || !ship.owner) return;
  if (!ship.on || !ship.retaliate || !ship.destruct) return;

  if (landed(ship)) {
    const auto &p = planets[ship.storbits][ship.pnumorbits];
    /* check to see if missiles/mines are present */
    auto sh2 = p->ships;
    while (sh2 && ship.destruct) {
      if (ships[sh2]->alive &&
          ((ships[sh2]->type == ShipType::STYPE_MISSILE) ||
           (ships[sh2]->type == ShipType::STYPE_MINE)) &&
          (ships[sh2]->owner != ship.owner) &&
          !(isset(races[ship.owner - 1].allied, ships[sh2]->owner) &&
            isset(races[ships[sh2]->owner - 1].allied, ship.owner))) {
        /* added last two tests to prevent mutually allied missiles
           getting shot up. */
        /* attack the missile/mine */
        auto numdest = retal_strength(ship);
        numdest = MIN(numdest, ship.destruct);
        numdest = MIN(numdest, ship.retaliate);
        ship.destruct -= numdest;
        auto const &s2sresult =
            shoot_ship_to_ship(ship, *ships[sh2], numdest, 0);
        auto [damange, short_buf, long_buf] = *s2sresult;
        push_telegram(ship.owner, ship.governor, long_buf);
        push_telegram(ships[sh2]->owner, ships[sh2]->governor, long_buf);
        post(short_buf, NewsType::COMBAT);
      }
      sh2 = ships[sh2]->nextship;
    }
  }
}

int do_weapon_plant(Ship &ship) {
  auto maxrate = (int)(races[ship.owner - 1].tech / 2.0);

  auto rate = round_rand(MIN((double)ship.resource / (double)RES_COST_WPLANT,
                             ship.fuel / FUEL_COST_WPLANT) *
                         (1. - .01 * (double)ship.damage) * (double)ship.popn /
                         (double)ship.max_crew);
  rate = std::min(rate, maxrate);
  use_resource(ship, (rate * RES_COST_WPLANT));
  use_fuel(ship, ((double)rate * FUEL_COST_WPLANT));
  return rate;
}
