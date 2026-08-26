// SPDX-License-Identifier: Apache-2.0

module;
#include <cstdlib>

import std;
import gblib;

module gblib;

std::expected<char, GroundMovementError> get_ground_order(const Ship& ship,
                                                          std::size_t index) {
  if (!std::holds_alternative<TerraformData>(ship.special())) {
    return std::unexpected(GroundMovementError::NotTerraformVehicle);
  }
  const auto& orders = ship.shipclass();
  if (orders.empty()) {
    return std::unexpected(GroundMovementError::EmptyOrders);
  }
  if (index >= orders.size()) {
    return std::unexpected(GroundMovementError::InvalidIndex);
  }
  const char order = orders[index];
  if (order == '\0') {
    return std::unexpected(GroundMovementError::EmptyOrders);
  }
  if (order == 's') {
    return std::unexpected(GroundMovementError::Stopped);
  }
  return order;
}

std::expected<Coordinates, GroundMovementError>
advance_ground_vehicle(Ship& ship, const Planet& planet,
                       EntityManager& entity_manager) {
  if (!std::holds_alternative<TerraformData>(ship.special())) {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::NotTerraformVehicle);
  }

  auto terraform = std::get<TerraformData>(ship.special());
  if (ship.shipclass().empty()) {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::EmptyOrders);
  }
  if (terraform.index >= ship.shipclass().size()) {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::InvalidIndex);
  }
  if (ship.shipclass()[terraform.index] == 's') {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::Stopped);
  }
  if (ship.shipclass()[terraform.index] == 'c') {
    terraform.index = 0; /* reset the orders */
    ship.special() = terraform;
    if (ship.shipclass().empty() || ship.shipclass()[0] == 'c') {
      ship.on() = 0;
      return std::unexpected(GroundMovementError::EmptyOrders);
    }
    if (ship.shipclass()[0] == 's') {
      ship.on() = 0;
      return std::unexpected(GroundMovementError::Stopped);
    }
  }

  const char order = ship.shipclass()[terraform.index];
  auto [x, y] = get_move(planet, order, ship.land_coords());

  bool bounced = false;

  if (y >= planet.Maxy()) {
    bounced = true;
    y -= 2; /* bounce off of south pole! */
  } else if (y < 0) {
    y = 1;
    bounced = true; /* bounce off of north pole! */
  }
  if (planet.Maxy() == 1) y = 0;

  if (terraform.index + 1 < ship.shipclass().size() &&
      ship.shipclass()[terraform.index + 1] != '\0') {
    ++terraform.index;
    if ((terraform.index + 1 >= ship.shipclass().size() ||
         ship.shipclass()[terraform.index + 1] == '\0') &&
        (!ship.notified())) {
      ship.notified() = 1;
      const std::string teleg_buf =
          std::format("%{0} is out of orders at %{1}.", ship,
                      prin_ship_orbits(entity_manager, ship));
      push_telegram(entity_manager, ship.owner(), ship.governor(), teleg_buf);
    }
    ship.special() = terraform;
  } else if (bounced) {
    if (terraform.index < ship.shipclass().size()) {
      ship.shipclass()[terraform.index] +=
          ((ship.shipclass()[terraform.index] > '5') ? -6 : 6);
    }
  }
  ship.set_land_coords({x, y});
  return Coordinates{x, y};
}

bool moveship_onplanet(Ship& ship, const Planet& planet,
                       EntityManager& entity_manager) {
  return advance_ground_vehicle(ship, planet, entity_manager).has_value();
}

std::expected<bool, GroundActionError>
execute_terraforming(Ship& ship, Planet& planet, SectorMap& smap,
                     EntityManager& entity_manager) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (!ship.popn()) return std::unexpected(GroundActionError::NoCrew);
  if (ship.fuel() < static_cast<double>(FUEL_COST_TERRA)) {
    if (!ship.notified()) {
      ship.notified() = 1;
      msg_OOF(entity_manager, ship);
    }
    return std::unexpected(GroundActionError::InsufficientFuel);
  }

  if (!moveship_onplanet(ship, planet, entity_manager)) {
    return std::unexpected(GroundActionError::MovementFailed);
  }

  auto& s = smap.get(ship.land_coords());
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return std::unexpected(GroundActionError::IncompatibleSector);

  if (s.get_condition() == race->likesbest) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" T{} is full of zealots!!!", ship.number()));
    return std::unexpected(GroundActionError::SectorAlreadyOptimal);
  }

  if (s.get_condition() == SectorType::SEC_GAS) {
    push_telegram(
        entity_manager, ship.owner(), ship.governor(),
        std::format(" T{} is trying to terraform gas.", ship.number()));
    return std::unexpected(GroundActionError::IncompatibleSector);
  }

  const int chance =
      (100 - static_cast<int>(ship.damage())) * ship.popn() / ship.max_crew();
  if (success(chance)) {
    /* only condition can be terraformed, type doesn't change */
    s.terraform(race->likesbest);
    use_fuel(ship, FUEL_COST_TERRA);
    if (success(50) && (planet.conditions(TOXIC) < 100)) {
      planet.conditions(TOXIC) += 1;
    }
    if ((ship.fuel() < static_cast<double>(FUEL_COST_TERRA)) &&
        (!ship.notified())) {
      ship.notified() = 1;
      msg_OOF(entity_manager, ship);
    }
    return true;
  }
  return false;
}

std::expected<int, GroundActionError>
execute_plowing(Ship& ship, Planet& planet, SectorMap& smap,
                EntityManager& entity_manager) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (ship.fuel() < static_cast<double>(FUEL_COST_PLOW)) {
    if (!ship.notified()) {
      ship.notified() = 1;
      msg_OOF(entity_manager, ship);
    }
    return std::unexpected(GroundActionError::InsufficientFuel);
  }

  if (!moveship_onplanet(ship, planet, entity_manager)) {
    return std::unexpected(GroundActionError::MovementFailed);
  }

  auto& s = smap.get(ship.land_coords());
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race || !race->likes[s.get_condition()]) {
    return std::unexpected(GroundActionError::IncompatibleSector);
  }
  if (s.get_fert() >= 100) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" K{} is full of zealots!!!", ship.number()));
    return std::unexpected(GroundActionError::SectorAlreadyOptimal);
  }

  int adjust = round_rand(10 *
                          (0.01 * (100.0 - static_cast<double>(ship.damage())) *
                           static_cast<double>(ship.popn())) /
                          ship.max_crew());
  s.set_fert(std::min(100U, s.get_fert() + adjust));
  if (s.get_fert() >= 100) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" K{} is full of zealots!!!", ship.number()));
  }
  use_fuel(ship, FUEL_COST_PLOW);
  if (success(50) && (planet.conditions(TOXIC) < 100)) {
    planet.conditions(TOXIC) += 1;
  }
  return adjust;
}

std::expected<int, GroundActionError>
upgrade_sector_dome(EntityManager& entity_manager, Ship& ship,
                    SectorMap& smap) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (ship.resource() < RES_COST_DOME) {
    return std::unexpected(GroundActionError::InsufficientResources);
  }

  auto& s = smap.get(ship.land_coords());
  if (s.get_eff() >= 100) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" Y{} is full of zealots!!!", ship.number()));
    return std::unexpected(GroundActionError::SectorAlreadyOptimal);
  }
  int adjust = round_rand(0.05 * (100.0 - static_cast<double>(ship.damage())) *
                          static_cast<double>(ship.popn()) / ship.max_crew());
  s.improve_efficiency(adjust);
  use_resource(ship, RES_COST_DOME);
  return adjust;
}

std::expected<int, GroundActionError>
strip_mine_quarry(Ship& ship, Planet& planet, SectorMap& smap,
                  EntityManager& entity_manager, TurnStats& stats) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (!ship.popn()) return std::unexpected(GroundActionError::NoCrew);
  if (ship.fuel() < static_cast<double>(FUEL_COST_QUARRY)) {
    if (!ship.notified()) {
      msg_OOF(entity_manager, ship);
      ship.notified() = 1;
    }
    ship.on() = 0;
    return std::unexpected(GroundActionError::InsufficientFuel);
  }

  auto& s = smap.get(ship.land_coords());
  /* nuke the sector */
  s.set_condition(SectorType::SEC_WASTED);
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return std::unexpected(GroundActionError::IncompatibleSector);

  int prod = round_rand(race->metabolism * static_cast<double>(ship.popn()) /
                        static_cast<double>(ship.max_crew()));
  ship.fuel() -= FUEL_COST_QUARRY;
  stats.prod_res[ship.owner()] += prod;
  int tox = int_rand(0, int_rand(0, prod));
  planet.conditions(TOXIC) = std::min(100, planet.conditions(TOXIC) + tox);
  if (s.get_fert() >= prod) {
    s.set_fert(s.get_fert() - prod);
  } else {
    s.set_fert(0);
  }
  return prod;
}

bool execute_berserker_bombardment(EntityManager& entity_manager, Ship& ship,
                                   Planet& planet) {
  if (ship.whatdest() != ScopeLevel::LEVEL_PLAN ||
      ship.whatorbits() != ScopeLevel::LEVEL_PLAN || landed(ship) ||
      ship.storbits() != ship.deststar() ||
      ship.pnumorbits() != ship.destpnum()) {
    return false;
  }

  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return false;

  int destroyed = berserker_bombard(entity_manager, ship, planet, *race);
  if (destroyed == 0) {
    const auto* dest_star = entity_manager.peek_star(ship.storbits());
    ship.destpnum() = int_rand(0, dest_star->numplanets() - 1);
    return false;
  }

  if (std::holds_alternative<MindData>(ship.special())) {
    auto mind = std::get<MindData>(ship.special());
    if (mind.who_killed.value > 0 && mind.who_killed.value <= MAXPLAYERS) {
      auto universe_handle = entity_manager.get_universe();
      if (universe_handle->VN_hitlist[mind.who_killed.value - 1] > 0) {
        --universe_handle->VN_hitlist[mind.who_killed.value - 1];
      }
    }
  }
  return true;
}

double refuel_gasgiant_orbiters(const Planet& planet, Ship& ship) {
  if (landed(ship) || planet.type() != PlanetType::GASGIANT) {
    return 0.0;
  }

  double fadd = 0.0;
  switch (ship.type()) {
    case ShipType::STYPE_TANKER:
      fadd = FUEL_GAS_ADD_TANKER;
      break;
    case ShipType::STYPE_HABITAT:
      fadd = FUEL_GAS_ADD_HABITAT;
      break;
    default:
      fadd = FUEL_GAS_ADD;
      break;
  }
  const double capacity = static_cast<double>(max_fuel(ship)) - ship.fuel();
  const double added = std::clamp(fadd, 0.0, std::max(0.0, capacity));
  if (added > 0.0) {
    rcv_fuel(ship, added);
  }
  return added;
}

void do_recover(EntityManager& entity_manager, const Star& star,
                Planet& planet) {
  int owners = 0;
  player_t i;
  player_t j;
  int stolenres = 0;
  int stolendes = 0;
  int stolenfuel = 0;
  int stolencrystals = 0;
  int all_buddies_here = 1;

  std::uint64_t ownerbits = 0;

  const planetnum_t planetnum = planet.planet_order();

  for (i = 1; i <= entity_manager.num_races() && all_buddies_here; i++) {
    if (planet.info(i).numsectsowned > 0) {
      owners++;
      setbit(ownerbits, i);
      for (j = 1; j < i && all_buddies_here; j++)
        if (isset(ownerbits, j)) {
          const auto* race_i = entity_manager.peek_race(i);
          const auto* race_j = entity_manager.peek_race(j);
          if (!race_i || !race_j || !isset(race_i->allied, j) ||
              !isset(race_j->allied, i))
            all_buddies_here = 0;
        }
    } else {        /* Player i owns no sectors */
      if (i != 1) { /* Can't steal from God */
        stolenres += planet.info(i).resource;
        stolendes += planet.info(i).destruct;
        stolenfuel += planet.info(i).fuel;
        stolencrystals += planet.info(i).crystals;
      }
    }
  }
  if (all_buddies_here && owners != 0 &&
      (stolenres > 0 || stolendes > 0 || stolenfuel > 0 ||
       stolencrystals > 0)) {
    /* Okay, we've got some loot to divvy up */
    int shares = owners;
    int res;
    int des;
    int fuel;
    int crystals;
    int givenres = 0;
    int givendes = 0;
    int givenfuel = 0;
    int givencrystals = 0;

    for (i = 1; i <= entity_manager.num_races(); i++)
      if (isset(ownerbits, i)) {
        std::stringstream telegram_buf;
        telegram_buf << std::format("Recovery Report: Planet /{}/{}\n",
                                    star.get_name(),
                                    star.get_planet_name(planetnum));
        push_telegram(entity_manager, i, star.governor(i), telegram_buf.str());
        telegram_buf.str("");
        telegram_buf << std::format("{:<14} {:>5} {:>5} {:>5} {:>5}\n", "",
                                    "res", "destr", "fuel", "xtal");
        push_telegram(entity_manager, i, star.governor(i), telegram_buf.str());
      }
    /* First: give the loot the the conquerers */
    for (i = 1; i <= entity_manager.num_races() && owners > 1; i++)
      if (isset(ownerbits, i)) { /* We have a winnah! */
        if ((res = round_rand((double)stolenres / shares)) + givenres >
            stolenres)
          res = stolenres - givenres;
        if ((des = round_rand((double)stolendes / shares)) + givendes >
            stolendes)
          des = stolendes - givendes;
        if ((fuel = round_rand((double)stolenfuel / shares)) + givenfuel >
            stolenfuel)
          fuel = stolenfuel - givenfuel;
        if ((crystals = round_rand((double)stolencrystals / shares)) +
                givencrystals >
            stolencrystals)
          crystals = stolencrystals - givencrystals;
        planet.info(i).resource += res;
        givenres += res;
        planet.info(i).destruct += des;
        givendes += des;
        planet.info(i).fuel += fuel;
        givenfuel += fuel;
        planet.info(i).crystals += crystals;
        givencrystals += crystals;

        owners--;
        {
          std::stringstream telegram_buf;
          const auto* race = entity_manager.peek_race(i);
          telegram_buf << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}",
                                      race->name, res, des, fuel, crystals);
          for (j = 1; j <= entity_manager.num_races(); j++) {
            if (isset(ownerbits, j)) {
              push_telegram(entity_manager, j, star.governor(j),
                            telegram_buf.str());
            }
          }
        }
      }
    /* Leftovers for last player */
    for (; i <= entity_manager.num_races(); i++)
      if (isset(ownerbits, i)) break;
    if (i <= entity_manager.num_races()) { /* It should be */
      res = stolenres - givenres;
      des = stolendes - givendes;
      fuel = stolenfuel - givenfuel;
      crystals = stolencrystals - givencrystals;

      planet.info(i).resource += res;
      planet.info(i).destruct += des;
      planet.info(i).fuel += fuel;
      planet.info(i).crystals += crystals;
      {
        std::stringstream first_telegram;
        const auto* race = entity_manager.peek_race(i);
        first_telegram << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}",
                                      race->name, res, des, fuel, crystals);
        std::stringstream second_telegram;
        second_telegram << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}\n",
                                       "Total:", stolenres, stolendes,
                                       stolenfuel, stolencrystals);
        for (j = 1; j <= entity_manager.num_races(); j++) {
          if (isset(ownerbits, j)) {
            push_telegram(entity_manager, j, star.governor(j),
                          first_telegram.str());
            push_telegram(entity_manager, j, star.governor(j),
                          second_telegram.str());
          }
        }
      }
    } else {
      push_telegram(entity_manager, 1, 0, "Bug in stealing resources\n");
    }
    /* Next: take all the loot away from the losers */
    for (i = 2; i <= entity_manager.num_races(); i++)
      if (!isset(ownerbits, i)) {
        planet.info(i).resource = 0;
        planet.info(i).destruct = 0;
        planet.info(i).fuel = 0;
        planet.info(i).crystals = 0;
      }
  }
}

void process_planetary_ships(EntityManager& entity_manager, Planet& planet,
                             SectorMap& smap, TurnStats& stats) {
  for (auto ship_handle : ShipList(entity_manager, planet.ships())) {
    auto& ship = *ship_handle;
    if (ship.alive() && !ship.rad()) {
      /* planet level functions - do these here because they use the sector map
              or affect planet production */
      switch (ship.type()) {
        case ShipType::OTYPE_VN:
          planet_doVN(ship, planet, smap, entity_manager, stats);
          break;
        case ShipType::OTYPE_BERS:
          if (!ship.destruct() || !ship.bombard())
            planet_doVN(ship, planet, smap, entity_manager, stats);
          else
            execute_berserker_bombardment(entity_manager, ship, planet);
          break;
        case ShipType::OTYPE_TERRA:
          execute_terraforming(ship, planet, smap, entity_manager);
          break;
        case ShipType::OTYPE_PLOW: {
          auto plow_res = execute_plowing(ship, planet, smap, entity_manager);
          if (!plow_res) {
            if (plow_res.error() == GroundActionError::NotLanded) {
              push_telegram(entity_manager, ship.owner(), ship.governor(),
                            std::format("K{} is not landed.", ship.number()));
            } else if (plow_res.error() == GroundActionError::NotSwitchedOn) {
              push_telegram(
                  entity_manager, ship.owner(), ship.governor(),
                  std::format("K{} is not switched on.", ship.number()));
            }
          }
          break;
        }
        case ShipType::OTYPE_DOME: {
          auto dome_res = upgrade_sector_dome(entity_manager, ship, smap);
          if (!dome_res) {
            if (dome_res.error() == GroundActionError::InsufficientResources) {
              push_telegram(entity_manager, ship.owner(), ship.governor(),
                            std::format("Y{} does not have enough resources.",
                                        ship.number()));
            } else if (dome_res.error() == GroundActionError::NotLanded) {
              push_telegram(entity_manager, ship.owner(), ship.governor(),
                            std::format("Y{} is not landed.", ship.number()));
            } else if (dome_res.error() == GroundActionError::NotSwitchedOn) {
              push_telegram(
                  entity_manager, ship.owner(), ship.governor(),
                  std::format("Y{} is not switched on.", ship.number()));
            }
          }
          break;
        }
        case ShipType::OTYPE_WPLANT:
          if (landed(ship))
            if (ship.resource() >= RES_COST_WPLANT &&
                ship.fuel() >= FUEL_COST_WPLANT)
              stats.prod_destruct[ship.owner()] +=
                  do_weapon_plant(ship, entity_manager);
            else {
              if (ship.resource() < RES_COST_WPLANT) {
                std::string buf = std::format(
                    "W{} does not have enough resources.", ship.number());
                push_telegram(entity_manager, ship.owner(), ship.governor(),
                              buf);
              } else {
                std::string buf = std::format("W{} does not have enough fuel.",
                                              ship.number());
                push_telegram(entity_manager, ship.owner(), ship.governor(),
                              buf);
              }
            }
          else {
            std::string buf = std::format("W{} is not landed.", ship.number());
            push_telegram(entity_manager, ship.owner(), ship.governor(), buf);
          }
          break;
        case ShipType::OTYPE_QUARRY: {
          auto quarry_res =
              strip_mine_quarry(ship, planet, smap, entity_manager, stats);
          if (!quarry_res) {
            std::string buf;
            if (quarry_res.error() == GroundActionError::NotSwitchedOn) {
              buf = std::format("q{} is not switched on.", ship.number());
            } else if (quarry_res.error() == GroundActionError::NotLanded) {
              buf = std::format("q{} is not landed.", ship.number());
            } else if (quarry_res.error() == GroundActionError::NoCrew) {
              buf = std::format("q{} does not have workers aboard.",
                                ship.number());
            }
            if (!buf.empty()) {
              push_telegram(entity_manager, ship.owner(), ship.governor(), buf);
            }
          }
          break;
        }
        default:
          break;
      }
      /* add fuel for ships orbiting a gas giant */
      refuel_gasgiant_orbiters(planet, ship);
    }
  }
}

double est_production(const Sector& s, EntityManager& entity_manager) {
  const auto* race = entity_manager.peek_race(s.get_owner());
  return (race->metabolism * (double)s.get_eff() * (double)s.get_eff() / 200.0);
}

int doplanet(EntityManager& entity_manager, const Star& star, Planet& planet,
             TurnStats& stats) {
  int nukex = 0;
  int nukey = 0;
  bool envir_damage = false;
  int o = 0;
  player_t i;
  int timer = 20;
  unsigned char allmod = 0;
  unsigned char allexp = 0;

  // Extract indices for array access and ship creation
  const starnum_t starnum = star.star_id();
  const planetnum_t planetnum = planet.planet_order();

  // Reset per-planet state in TurnStats
  // Note: TurnStats is reused across planets, so we reset per-planet fields
  // here
  stats.Sectinfo = {};
  stats.Claims = false;

  planet.maxpopn() = 0;

  planet.popn() = 0; /* initialize population for recount */
  planet.troops() = 0;
  planet.total_resources() = 0;

  /* reset global variables */
  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    stats.Compat[p] = planet.compatibility(*race);
    planet.info(p).numsectsowned = 0;
    planet.info(p).troops = 0;
    planet.info(p).popn = 0;
    planet.info(p).est_production = 0.0;
    stats.prod_crystals[p] = 0;
    stats.prod_fuel[p] = 0;
    stats.prod_destruct[p] = 0;
    stats.prod_res[p] = 0;
    stats.avg_mob[p] = 0;
  }

  auto smap_handle = entity_manager.get_sectormap(starnum, planetnum);
  if (!smap_handle.get()) {
    return 0;
  }
  auto& smap = *smap_handle;
  process_planetary_ships(entity_manager, planet, smap, stats);

  /* check for space mirrors (among other things) warming the planet */
  /* if a change in any artificial warming/cooling trends */
  planet.update_climate(stats.Stinfo[starnum.value][planetnum.value].temp_add);

  for (Sector& p : smap.shuffle()) {
    if (p.get_owner() != 0 && (p.get_popn() || p.get_troops())) {
      allmod = 1;
      if (!star.nova_stage()) {
        produce(entity_manager, star, planet, p, stats);
        if (p.get_owner() != 0)
          planet.info(p.get_owner()).est_production +=
              est_production(p, entity_manager);
        spread(entity_manager, planet, p, smap, stats);
      } else {
        p.apply_supernova(star.nova_stage());
      }
      stats.Sectinfo[p.get_x()][p.get_y()].done = true;
    }

    p.clear_owner_if_empty();

    /*
        if (p->wasted) {
            if (x>1 && x<planet->Maxx-2) {
                if (p->des==DES_SEA || p->des==DES_GAS) {
                    if ( y>1 && y<planet->Maxy-2 &&
                        (!(p-1)->wasted || !(p+1)->wasted) && !random()%5)
                        p->wasted = 0;
                } else if (p->des==DES_LAND || p->des==DES_MOUNT
                           || p->des==DES_ICE) {
                    if ( y>1 && y<planet->Maxy-2 && ((p-1)->popn || (p+1)->popn)
                        && !random()%10)
                        p->wasted = 0;
                }
            }
        }
    */
    /*
        if (entity_manager.peek_star(starnum)->nova_stage) {
            if (p->des==DES_ICE)
                if(random()&01)
                    p->des = DES_LAND;
                else if (p->des==DES_SEA)
                    if(random()&01)
                        if ( (x>0 && (p-1)->des==DES_LAND) ||
                            (x<planet->Maxx-1 && (p+1)->des==DES_LAND) ||
                            (y>0 && (p-planet->Maxx)->des==DES_LAND) ||
                            (y<planet->Maxy-1 && (p+planet->Maxx)->des==DES_LAND
       ) ) {
                            p->des = DES_LAND;
                            p->popn = p->owner = p->troops = 0;
                            p->resource += int_rand(1,5);
                            p->fert = int_rand(1,4);
                        }
                        }
                        */
  }

  for (const auto& p : smap.owned()) {
    planet.info(p.get_owner()).numsectsowned++;
  }

  if (planet.expltimer() >= 1) planet.expltimer() -= 1;
  if (!star.nova_stage() && !planet.expltimer()) {
    if (!planet.expltimer()) planet.expltimer() = 5;
    for (i = 1; !stats.Claims && !allexp && i <= entity_manager.num_races();
         i++) {
      /* sectors have been modified for this player*/
      if (planet.info(i).numsectsowned > 0)
        while (!stats.Claims && !allexp && timer > 0) {
          timer -= 1;
          o = 1;
          for (Sector& p : smap.shuffle()) {
            /* find out if all sectors have been explored */
            o &= (stats.Sectinfo[p.get_x()][p.get_y()].explored != player_t{0});
            const auto* explore_race = entity_manager.peek_race(i);
            if (((stats.Sectinfo[p.get_x()][p.get_y()].explored == i) &&
                 success(50)) &&
                (p.get_owner() == 0 &&
                 p.get_condition() != SectorType::SEC_WASTED &&
                 p.get_condition() == explore_race->likesbest)) {
              /*  explorations have found an island */
              stats.Claims = true;
              p.colonize(i, explore_race->number_sexes);
              stats.tot_captured = 1;
              break;
            } else {
              explore(planet, p, p.get_x(), p.get_y(), i, stats);
            }
          }
          allexp |= o; /* all sectors explored for this player */
        }
    }
  }

  if (allexp) planet.expltimer() = 5;

  /* environment nukes a random sector */
  if (planet.conditions(TOXIC) > ENVIR_DAMAGE_TOX) {
    envir_damage = true;
    nukex = int_rand(0, (int)planet.Maxx() - 1);
    nukey = int_rand(0, (int)planet.Maxy() - 1);
    auto& p = smap.get(nukex, nukey);
    p.devastate();
  }

  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    planet.info(p).prod_crystals = stats.prod_crystals[p];
    planet.info(p).prod_res = stats.prod_res[p];
    planet.info(p).prod_fuel = stats.prod_fuel[p];
    planet.info(p).prod_dest = stats.prod_destruct[p];
    if (planet.info(p).autorep) {
      planet.info(p).autorep--;
      std::stringstream telegram_buf;
      telegram_buf << std::format("\nFrom /{}/{}\n", star.get_name(),
                                  star.get_planet_name(planetnum));

      if (stats.Stinfo[starnum.value][planetnum.value].temp_add) {
        telegram_buf << std::format("Temp: {} to {}\n",
                                    planet.conditions(RTEMP),
                                    planet.conditions(TEMP));
      }
      telegram_buf << std::format("Total      Prod: {}r {}f {}d\n",
                                  stats.prod_res[p], stats.prod_fuel[p],
                                  stats.prod_destruct[p]);
      if (stats.prod_crystals[p]) {
        telegram_buf << std::format("    {} crystals found\n",
                                    stats.prod_crystals[p]);
      }
      if (stats.tot_captured) {
        telegram_buf << std::format("{} sectors captured\n",
                                    stats.tot_captured);
      }
      if (star.nova_stage()) {
        telegram_buf << std::format(
            "This planet's primary is in a Stage {} nova.\n",
            star.nova_stage());
      }
      /* remind the player that he should clean up the environment. */
      if (envir_damage) {
        telegram_buf << std::format("Environmental damage on sector {},{}\n",
                                    nukex, nukey);
      }
      if (planet.slaved_to() != 0) {
        telegram_buf << std::format("ENSLAVED to player {}\n",
                                    planet.slaved_to());
      }
      push_telegram(entity_manager, i, star.governor(i), telegram_buf.str());
    }
  }

  /* find out who is on this planet, for nova notification */
  if (star.nova_stage() == 1) {
    {
      std::stringstream telegram_buf;
      telegram_buf << std::format("BULLETIN from /{}/{}\n", star.get_name(),
                                  star.get_planet_name(planetnum));
      telegram_buf << std::format("\nStar {} is undergoing nova.\n",
                                  star.get_name());
      if (planet.type() == PlanetType::EARTH ||
          planet.type() == PlanetType::WATER ||
          planet.type() == PlanetType::FOREST) {
        telegram_buf << "Seas and rivers are boiling!\n";
      }
      telegram_buf << "This planet must be evacuated immediately!\n"
                   << TELEG_DELIM;
      for (i = 1; i <= entity_manager.num_races(); i++) {
        if (planet.info(i).numsectsowned) {
          push_telegram(entity_manager, i, star.governor(i),
                        telegram_buf.str());
        }
      }
    }
  }

  do_recover(entity_manager, star, planet);

  planet.popn() = 0;
  planet.troops() = 0;
  planet.maxpopn() = 0;
  planet.total_resources() = 0;

  for (i = 1; i <= entity_manager.num_races(); i++) {
    planet.info(i).numsectsowned = 0;
    planet.info(i).popn = 0;
    planet.info(i).troops = 0;
  }

  for (Sector& p : smap.shuffle()) {
    if (p.get_owner() != 0) {
      planet.info(p.get_owner()).numsectsowned++;
      planet.info(p.get_owner()).troops += p.get_troops();
      planet.info(p.get_owner()).popn += p.get_popn();
      planet.popn() += p.get_popn();
      planet.troops() += p.get_troops();
      const auto* owner_race = entity_manager.peek_race(p.get_owner());
      planet.maxpopn() +=
          maxsupport(*owner_race, p, stats.Compat[p.get_owner()],
                     planet.conditions(TOXIC));
      stats.Power[p.get_owner()].troops += p.get_troops();
      stats.Power[p.get_owner()].popn += p.get_popn();
      stats.Power[p.get_owner()].sum_eff += p.get_eff();
      stats.Power[p.get_owner()].sum_mob += p.get_mobilization();
      stats.starpopns[starnum.value][p.get_owner()] += p.get_popn();
    } else {
      p.clear_popn();
      p.clear_troops();
    }
    planet.total_resources() += p.get_resource();
  }

  /* deal with enslaved planets */
  if (planet.is_enslaved()) {
    if (!planet.is_slave_revolt_triggered()) {
      for (const Race* race : RaceList::readonly(entity_manager)) {
        const player_t p = race->Playernum;
        /* add production to slave holder of planet */
        if (planet.info(p).numsectsowned) {
          planet.info(planet.slaved_to()).resource += stats.prod_res[p];
          stats.prod_res[p] = 0;
          planet.info(planet.slaved_to()).fuel += stats.prod_fuel[p];
          stats.prod_fuel[p] = 0;
          planet.info(planet.slaved_to()).destruct += stats.prod_destruct[p];
          stats.prod_destruct[p] = 0;
        }
      }
    } else {
      /* slave revolt! */
      /* first nuke some random sectors from the revolt */
      int revolt_sectors = planet.calculate_revolt_devastation_count();
      while (--revolt_sectors) {
        auto& p = smap.get(int_rand(0, (int)planet.Maxx() - 1),
                           int_rand(0, (int)planet.Maxy() - 1));
        if (p.get_popn() + p.get_troops()) {
          p.devastate();
        }
      }
      /* now nuke all sectors belonging to former master */
      for (Sector& p : smap.shuffle()) {
        if (stats.Stinfo[starnum.value][planetnum.value].intimidated &&
            success(50)) {
          if (p.get_owner() == planet.slaved_to()) {
            p.devastate();
          }
        }
        /* also add up the populations while here */
      }
      {
        std::stringstream telegram_buf;
        telegram_buf << std::format(
            "\nThere has been a SLAVE REVOLT on /{}/{}!\n", star.get_name(),
            star.get_planet_name(planetnum));
        telegram_buf << std::format(
            "All population belonging to player #{} on the planet have been "
            "killed!\n",
            planet.slaved_to());
        telegram_buf << "Productions now go to their rightful owners.\n";
        for (const Race* race : RaceList::readonly(entity_manager)) {
          const player_t r_id = race->Playernum;
          if (planet.info(r_id).numsectsowned) {
            push_telegram(entity_manager, r_id, star.governor(r_id),
                          telegram_buf.str());
          }
        }
      }
      planet.free_slaves();
    }
  }

  /* add production to all people here */
  for (auto race_handle : RaceList(entity_manager)) {
    auto& race = *race_handle;
    const player_t player = race.Playernum;
    auto& info = planet.info(player);
    if (info.numsectsowned > 0) {
      info.deposit_production(stats.prod_fuel[player], stats.prod_res[player],
                              stats.prod_destruct[player],
                              stats.prod_crystals[player]);

      const auto gov_idx = star.governor(player);
      auto& gov = race.governor[gov_idx.value];

      /* tax the population - set new tax rate when done */
      info.collect_tax(gov, race);

      /* do tech investments */
      info.invest_tech(gov, race);

      /* build wc's if it's been ordered */
      if (planet.info(player).tox_thresh.has_value() &&
          planet.conditions(TOXIC) >= *planet.info(player).tox_thresh &&
          planet.info(player).resource >=
              Shipcost(ShipType::OTYPE_TOXWC, race)) {
        int t = std::min(TOXMAX, planet.conditions(TOXIC));
        planet.conditions(TOXIC) -= t;

        // Create new ship via EntityManager with designated initializers
        ship_struct s2{
            .owner = player,
            .governor = star.governor(player),
            .xpos = star.xpos() + planet.xpos(),
            .ypos = star.ypos() + planet.ypos(),
            .mass = 1.0,
            .land_coords = Coordinates{int_rand(0, (int)planet.Maxx() - 1),
                                       int_rand(0, (int)planet.Maxy() - 1)},
            .armor = static_cast<unsigned char>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_ARMOR]),
            .max_crew = static_cast<unsigned short>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_MAXCREW]),
            .max_resource = static_cast<resource_t>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_CARGO]),
            .max_destruct = static_cast<unsigned short>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_DESTCAP]),
            .max_fuel = static_cast<unsigned short>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_FUELCAP]),
            .max_speed = static_cast<unsigned short>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_SPEED]),
            .build_cost = static_cast<unsigned short>(
                Shipcost(ShipType::OTYPE_TOXWC, race)),
            .base_mass = 1.0,
            .special = WasteData{.toxic = static_cast<unsigned char>(t)},
            .storbits = starnum,
            .deststar = starnum,
            .destpnum = planetnum,
            .pnumorbits = planetnum,
            .whatdest = ScopeLevel::LEVEL_PLAN,
            .whatorbits = ScopeLevel::LEVEL_PLAN,
            .type = ShipType::OTYPE_TOXWC,
            .active = 1,
            .alive = 1,
            .docked = 1,
            .guns = GTYPE_NONE,
            .primary = static_cast<unsigned long>(
                Shipdata[ShipType::OTYPE_TOXWC][ABIL_GUNS]),
            .primtype = shipdata_primary(ShipType::OTYPE_TOXWC),
            .sectype = shipdata_secondary(ShipType::OTYPE_TOXWC),
        };
        auto ship_handle = entity_manager.create_ship(s2);
        Ship& ship = *ship_handle;
        ship.name() = std::format("Scum{:04d}", ship.number());
        ship.size() = ship_size(ship);

        insert_sh_plan(planet, &ship);
      }
    }
  } /* (if numsectsowned) */

  if (planet.maxpopn() > 0 && planet.conditions(TOXIC) < 100)
    planet.conditions(TOXIC) += planet.popn() / planet.maxpopn();

  if (planet.conditions(TOXIC) > 100)
    planet.conditions(TOXIC) = 100;
  else if (planet.conditions(TOXIC) < 0)
    planet.conditions(TOXIC) = 0;

  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    auto& info = planet.info(p);
    stats.Power[p].resource += info.resource;
    stats.Power[p].destruct += info.destruct;
    stats.Power[p].fuel += info.fuel;
    stats.Power[p].sectors_owned += info.numsectsowned;
    stats.Power[p].planets_owned += !!info.numsectsowned;
    info.update_combat_readiness(stats.avg_mob[p]);
  }
  return allmod;
}
