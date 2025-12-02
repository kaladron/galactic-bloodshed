// SPDX-License-Identifier: Apache-2.0

/*  doplanet.c -- do one turn on a planet. */

module;

import gblib;
import std.compat;

#include <strings.h>

#include <cstdlib>

module gblib;

namespace {
bool moveship_onplanet(Ship& ship, const Planet& planet) {
  if (!std::holds_alternative<TerraformData>(ship.special())) {
    ship.on() = 0;
    return false;
  }

  auto terraform = std::get<TerraformData>(ship.special());
  if (ship.shipclass()[terraform.index] == 's') {
    ship.on() = 0;
    return false;
  }
  if (ship.shipclass()[terraform.index] == 'c') {
    terraform.index = 0; /* reset the orders */
    ship.special() = terraform;
  }

  auto [x, y] = get_move(planet, ship.shipclass()[terraform.index],
                         {ship.land_x(), ship.land_y()});

  bool bounced = false;

  if (y >= planet.Maxy()) {
    bounced = true;
    y -= 2; /* bounce off of south pole! */
  } else if (y < 0)
    y = 1;
  bounced = true; /* bounce off of north pole! */
  if (planet.Maxy() == 1) y = 0;
  if (ship.shipclass()[terraform.index + 1] != '\0') {
    ++terraform.index;
    if ((ship.shipclass()[terraform.index + 1] == '\0') && (!ship.notified())) {
      ship.notified() = 1;
      std::string teleg_buf =
          std::format("%{0} is out of orders at %{1}.", ship_to_string(ship),
                      prin_ship_orbits(ship));
      push_telegram(ship.owner(), ship.governor(), teleg_buf);
    }
    ship.special() = terraform;
  } else if (bounced) {
    ship.shipclass()[terraform.index] +=
        ((ship.shipclass()[terraform.index] > '5') ? -6 : 6);
  }
  ship.land_x() = x;
  ship.land_y() = y;
  return true;
}

// move, and then terraform
void terraform(Ship& ship, Planet& planet, SectorMap& smap,
               EntityManager& entity_manager) {
  if (!moveship_onplanet(ship, planet)) return;
  auto& s = smap.get(ship.land_x(), ship.land_y());

  const auto* race = entity_manager.peek_race(ship.owner());
  if (s.get_condition() == race->likesbest) {
    std::string buf = std::format(" T{} is full of zealots!!!", ship.number());
    push_telegram(ship.owner(), ship.governor(), buf);
    return;
  }

  if (s.get_condition() == SectorType::SEC_GAS) {
    std::string buf =
        std::format(" T{} is trying to terraform gas.", ship.number());
    push_telegram(ship.owner(), ship.governor(), buf);
    return;
  }

  if (success((100 - (int)ship.damage()) * ship.popn() / ship.max_crew())) {
    /* only condition can be terraformed, type doesn't change */
    s.set_condition(race->likesbest);
    s.set_eff(0);
    s.set_mobilization(0);
    s.set_popn(0);
    s.set_troops(0);
    s.set_owner(0);
    use_fuel(ship, FUEL_COST_TERRA);
    if ((random() & 01) && (planet.conditions(TOXIC) < 100))
      planet.conditions(TOXIC) += 1;
    if ((ship.fuel() < (double)FUEL_COST_TERRA) && (!ship.notified())) {
      ship.notified() = 1;
      msg_OOF(ship);
    }
  }
}

void plow(Ship* ship, Planet& planet, SectorMap& smap,
          EntityManager& entity_manager) {
  if (!moveship_onplanet(*ship, planet)) return;
  auto& s = smap.get(ship->land_x(), ship->land_y());
  const auto* race = entity_manager.peek_race(ship->owner());
  if ((race->likes[s.get_condition()]) && (s.get_fert() < 100)) {
    int adjust = round_rand(
        10 * (0.01 * (100.0 - (double)ship->damage()) * (double)ship->popn()) /
        ship->max_crew());
    if ((ship->fuel() < (double)FUEL_COST_PLOW) && (!ship->notified())) {
      ship->notified() = 1;
      msg_OOF(*ship);
      return;
    }
    s.set_fert(std::min(100U, s.get_fert() + adjust));
    if (s.get_fert() >= 100) {
      std::string buf =
          std::format(" K{} is full of zealots!!!", ship->number());
      push_telegram(ship->owner(), ship->governor(), buf);
    }
    use_fuel(*ship, FUEL_COST_PLOW);
    if ((random() & 01) && (planet.conditions(TOXIC) < 100))
      planet.conditions(TOXIC) += 1;
  }
}

void do_dome(Ship* ship, SectorMap& smap) {
  auto& s = smap.get(ship->land_x(), ship->land_y());
  if (s.get_eff() >= 100) {
    std::string buf = std::format(" Y{} is full of zealots!!!", ship->number());
    push_telegram(ship->owner(), ship->governor(), buf);
    return;
  }
  int adjust = round_rand(.05 * (100. - (double)ship->damage()) *
                          (double)ship->popn() / ship->max_crew());
  s.set_eff(s.get_eff() + adjust);
  s.set_eff(std::min<unsigned int>(s.get_eff(), 100));
  use_resource(*ship, RES_COST_DOME);
}

void do_quarry(Ship* ship, Planet& planet, SectorMap& smap,
               EntityManager& entity_manager) {
  auto& s = smap.get(ship->land_x(), ship->land_y());

  if ((ship->fuel() < (double)FUEL_COST_QUARRY)) {
    if (!ship->notified()) msg_OOF(*ship);
    ship->notified() = 1;
    return;
  }
  /* nuke the sector */
  s.set_condition(SectorType::SEC_WASTED);
  const auto* race = entity_manager.peek_race(ship->owner());
  int prod = round_rand(race->metabolism * (double)ship->popn() /
                        (double)ship->max_crew());
  ship->fuel() -= FUEL_COST_QUARRY;
  prod_res[ship->owner() - 1] += prod;
  int tox = int_rand(0, int_rand(0, prod));
  planet.conditions(TOXIC) = std::min(100, planet.conditions(TOXIC) + tox);
  if (s.get_fert() >= prod)
    s.set_fert(s.get_fert() - prod);
  else
    s.set_fert(0);
}

void do_berserker(EntityManager& entity_manager, Ship* ship, Planet& planet) {
  if (ship->whatdest() == ScopeLevel::LEVEL_PLAN &&
      ship->whatorbits() == ScopeLevel::LEVEL_PLAN && !landed(*ship) &&
      ship->storbits() == ship->deststar() &&
      ship->pnumorbits() == ship->destpnum()) {
    const auto* race = entity_manager.peek_race(ship->owner());
    if (!berserker_bombard(entity_manager, *ship, planet, *race)) {
      const auto* dest_star = entity_manager.peek_star(ship->storbits());
      ship->destpnum() = int_rand(0, dest_star->numplanets() - 1);
    } else if (std::holds_alternative<MindData>(ship->special())) {
      auto mind = std::get<MindData>(ship->special());
      if (Sdata.VN_hitlist[mind.who_killed - 1] > 0)
        --Sdata.VN_hitlist[mind.who_killed - 1];
    }
  }
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

  uint64_t ownerbits = 0;

  const planetnum_t planetnum = planet.planet_order();

  for (i = 1; i <= Num_races && all_buddies_here; i++) {
    if (planet.info(i - 1).numsectsowned > 0) {
      owners++;
      setbit(ownerbits, i);
      for (j = 1; j < i && all_buddies_here; j++)
        if (isset(ownerbits, j)) {
          const auto* race_i = entity_manager.peek_race(i);
          const auto* race_j = entity_manager.peek_race(j);
          if (!isset(race_i->allied, j) || !isset(race_j->allied, i))
            all_buddies_here = 0;
        }
    } else {        /* Player i owns no sectors */
      if (i != 1) { /* Can't steal from God */
        stolenres += planet.info(i - 1).resource;
        stolendes += planet.info(i - 1).destruct;
        stolenfuel += planet.info(i - 1).fuel;
        stolencrystals += planet.info(i - 1).crystals;
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

    for (i = 1; i <= Num_races; i++)
      if (isset(ownerbits, i)) {
        std::stringstream telegram_buf;
        telegram_buf << std::format("Recovery Report: Planet /{}/{}\n",
                                    star.get_name(),
                                    star.get_planet_name(planetnum));
        push_telegram(i, star.governor(i - 1), telegram_buf.str());
        telegram_buf.str("");
        telegram_buf << std::format("{:<14} {:>5} {:>5} {:>5} {:>5}\n", "",
                                    "res", "destr", "fuel", "xtal");
        push_telegram(i, star.governor(i - 1), telegram_buf.str());
      }
    /* First: give the loot the the conquerers */
    for (i = 1; i <= Num_races && owners > 1; i++)
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
        planet.info(i - 1).resource += res;
        givenres += res;
        planet.info(i - 1).destruct += des;
        givendes += des;
        planet.info(i - 1).fuel += fuel;
        givenfuel += fuel;
        planet.info(i - 1).crystals += crystals;
        givencrystals += crystals;

        owners--;
        {
          std::stringstream telegram_buf;
          const auto* race = entity_manager.peek_race(i);
          telegram_buf << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}",
                                      race->name, res, des, fuel, crystals);
          for (j = 1; j <= Num_races; j++) {
            if (isset(ownerbits, j)) {
              push_telegram(j, star.governor(j - 1), telegram_buf.str());
            }
          }
        }
      }
    /* Leftovers for last player */
    for (; i <= Num_races; i++)
      if (isset(ownerbits, i)) break;
    if (i <= Num_races) { /* It should be */
      res = stolenres - givenres;
      des = stolendes - givendes;
      fuel = stolenfuel - givenfuel;
      crystals = stolencrystals - givencrystals;

      planet.info(i - 1).resource += res;
      planet.info(i - 1).destruct += des;
      planet.info(i - 1).fuel += fuel;
      planet.info(i - 1).crystals += crystals;
      {
        std::stringstream first_telegram;
        const auto* race = entity_manager.peek_race(i);
        first_telegram << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}",
                                      race->name, res, des, fuel, crystals);
        std::stringstream second_telegram;
        second_telegram << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}\n",
                                       "Total:", stolenres, stolendes,
                                       stolenfuel, stolencrystals);
        for (j = 1; j <= Num_races; j++) {
          if (isset(ownerbits, j)) {
            push_telegram(j, star.governor(j - 1), first_telegram.str());
            push_telegram(j, star.governor(j - 1), second_telegram.str());
          }
        }
      }
    } else {
      push_telegram(1, 0, "Bug in stealing resources\n");
    }
    /* Next: take all the loot away from the losers */
    for (i = 2; i <= Num_races; i++)
      if (!isset(ownerbits, i)) {
        planet.info(i - 1).resource = 0;
        planet.info(i - 1).destruct = 0;
        planet.info(i - 1).fuel = 0;
        planet.info(i - 1).crystals = 0;
      }
  }
}

double est_production(const Sector& s, EntityManager& entity_manager) {
  const auto* race = entity_manager.peek_race(s.get_owner());
  return (race->metabolism * (double)s.get_eff() * (double)s.get_eff() / 200.0);
}
}  // namespace

int doplanet(EntityManager& entity_manager, const Star& star, Planet& planet,
             TurnStats& stats) {
  int shipno;
  int nukex;
  int nukey;
  int o = 0;
  int i;
  Ship* ship;
  double fadd;
  int timer = 20;
  unsigned char allmod = 0;
  unsigned char allexp = 0;

  // Extract indices for array access and ship creation
  const starnum_t starnum = star.star_id();
  const planetnum_t planetnum = planet.planet_order();

  bzero((char*)Sectinfo, sizeof(Sectinfo));

  bzero((char*)avg_mob, sizeof(avg_mob));
  bzero((char*)prod_res, sizeof(prod_res));
  bzero((char*)prod_fuel, sizeof(prod_fuel));
  bzero((char*)prod_destruct, sizeof(prod_destruct));
  bzero((char*)prod_crystals, sizeof(prod_crystals));

  tot_resdep = prod_eff = prod_mob = tot_captured = 0;
  Claims = 0;

  planet.maxpopn() = 0;

  planet.popn() = 0; /* initialize population for recount */
  planet.troops() = 0;
  planet.total_resources() = 0;

  /* reset global variables */
  for (i = 1; i <= Num_races; i++) {
    const auto* race = entity_manager.peek_race(i);
    Compat[i - 1] = planet.compatibility(*race);
    planet.info(i - 1).numsectsowned = 0;
    planet.info(i - 1).troops = 0;
    planet.info(i - 1).popn = 0;
    planet.info(i - 1).est_production = 0.0;
    prod_crystals[i - 1] = 0;
    prod_fuel[i - 1] = 0;
    prod_destruct[i - 1] = 0;
    prod_res[i - 1] = 0;
    avg_mob[i - 1] = 0;
  }

  auto smap = getsmap(planet);
  shipno = planet.ships();
  while (shipno) {
    ship = ships[shipno];
    if (ship->alive() && !ship->rad()) {
      /* planet level functions - do these here because they use the sector map
              or affect planet production */
      switch (ship->type()) {
        case ShipType::OTYPE_VN:
          planet_doVN(*ship, planet, smap);
          break;
        case ShipType::OTYPE_BERS:
          if (!ship->destruct() || !ship->bombard())
            planet_doVN(*ship, planet, smap);
          else
            do_berserker(entity_manager, ship, planet);
          break;
        case ShipType::OTYPE_TERRA:
          if ((ship->on() && landed(*ship) && ship->popn())) {
            if (ship->fuel() >= (double)FUEL_COST_TERRA)
              terraform(*ship, planet, smap, entity_manager);
            else if (!ship->notified()) {
              ship->notified() = 1;
              msg_OOF(*ship);
            }
          }
          break;
        case ShipType::OTYPE_PLOW:
          if (ship->on() && landed(*ship)) {
            if (ship->fuel() >= (double)FUEL_COST_PLOW)
              plow(ship, planet, smap, entity_manager);
            else if (!ship->notified()) {
              ship->notified() = 1;
              msg_OOF(*ship);
            }
          } else if (ship->on()) {
            std::string buf = std::format("K{} is not landed.", ship->number());
            push_telegram(ship->owner(), ship->governor(), buf);
          } else {
            std::string buf =
                std::format("K{} is not switched on.", ship->number());
            push_telegram(ship->owner(), ship->governor(), buf);
          }
          break;
        case ShipType::OTYPE_DOME:
          if (ship->on() && landed(*ship)) {
            if (ship->resource() >= RES_COST_DOME)
              do_dome(ship, smap);
            else {
              std::string buf = std::format(
                  "Y{} does not have enough resources.", ship->number());
              push_telegram(ship->owner(), ship->governor(), buf);
            }
          } else if (ship->on()) {
            std::string buf = std::format("Y{} is not landed.", ship->number());
            push_telegram(ship->owner(), ship->governor(), buf);
          } else {
            std::string buf =
                std::format("Y{} is not switched on.", ship->number());
            push_telegram(ship->owner(), ship->governor(), buf);
          }
          break;
        case ShipType::OTYPE_WPLANT:
          if (landed(*ship))
            if (ship->resource() >= RES_COST_WPLANT &&
                ship->fuel() >= FUEL_COST_WPLANT)
              prod_destruct[ship->owner() - 1] +=
                  do_weapon_plant(*ship, entity_manager);
            else {
              if (ship->resource() < RES_COST_WPLANT) {
                std::string buf = std::format(
                    "W{} does not have enough resources.", ship->number());
                push_telegram(ship->owner(), ship->governor(), buf);
              } else {
                std::string buf = std::format("W{} does not have enough fuel.",
                                              ship->number());
                push_telegram(ship->owner(), ship->governor(), buf);
              }
            }
          else {
            std::string buf = std::format("W{} is not landed.", ship->number());
            push_telegram(ship->owner(), ship->governor(), buf);
          }
          break;
        case ShipType::OTYPE_QUARRY:
          if ((ship->on() && landed(*ship) && ship->popn())) {
            if (ship->fuel() >= FUEL_COST_QUARRY)
              do_quarry(ship, planet, smap, entity_manager);
            else if (!ship->notified()) {
              ship->on() = 0;
              msg_OOF(*ship);
            }
          } else {
            std::string buf;
            if (!ship->on()) {
              buf = std::format("q{} is not switched on.", ship->number());
            }
            if (!landed(*ship)) {
              buf = std::format("q{} is not landed.", ship->number());
            }
            if (!ship->popn()) {
              buf = std::format("q{} does not have workers aboard.",
                                ship->number());
            }
            push_telegram(ship->owner(), ship->governor(), buf);
          }
          break;
        default:
          break;
      }
      /* add fuel for ships orbiting a gas giant */
      if (!landed(*ship) && planet.type() == PlanetType::GASGIANT) {
        switch (ship->type()) {
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
        fadd = std::min((double)max_fuel(*ship) - ship->fuel(), fadd);
        rcv_fuel(*ship, fadd);
      }
    }
    shipno = ship->nextship();
  }

  /* check for space mirrors (among other things) warming the planet */
  /* if a change in any artificial warming/cooling trends */
  planet.conditions(TEMP) = planet.conditions(RTEMP) +
                            stats.Stinfo[starnum][planetnum].temp_add +
                            int_rand(-5, 5);

  for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
    Sector& p = sector_wrap;
    if (p.get_owner() && (p.get_popn() || p.get_troops())) {
      allmod = 1;
      if (!star.nova_stage()) {
        produce(star, planet, p);
        if (p.get_owner())
          planet.info(p.get_owner() - 1).est_production +=
              est_production(p, entity_manager);
        spread(planet, p, smap);
      } else {
        /* damage sector from supernova */
        p.set_resource(p.get_resource() + 1);
        p.set_fert(p.get_fert() * 0.8);
        if (star.nova_stage() == 14) {
          p.set_popn(0);
          p.set_owner(0);
          p.set_troops(0);
        } else
          p.set_popn(round_rand((double)p.get_popn() * .50));
      }
      Sectinfo[p.get_x()][p.get_y()].done = 1;
    }

    if ((!p.get_popn() && !p.get_troops()) || !p.get_owner()) {
      p.set_owner(0);
      p.set_popn(0);
      p.set_troops(0);
    }

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
        if (stars[starnum].nova_stage) {
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

  for (auto& p : smap) {
    if (p.get_owner()) planet.info(p.get_owner() - 1).numsectsowned++;
  }

  if (planet.expltimer() >= 1) planet.expltimer() -= 1;
  if (!star.nova_stage() && !planet.expltimer()) {
    if (!planet.expltimer()) planet.expltimer() = 5;
    for (i = 1; !Claims && !allexp && i <= Num_races; i++) {
      /* sectors have been modified for this player*/
      if (planet.info(i - 1).numsectsowned > 0)
        while (!Claims && !allexp && timer > 0) {
          timer -= 1;
          o = 1;
          for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
            if (!Claims) break;
            Sector& p = sector_wrap;
            /* find out if all sectors have been explored */
            o &= Sectinfo[p.get_x()][p.get_y()].explored;
            const auto* explore_race = entity_manager.peek_race(i);
            if (((Sectinfo[p.get_x()][p.get_y()].explored == i) &&
                 !(random() & 02)) &&
                (!p.get_owner() &&
                 p.get_condition() != SectorType::SEC_WASTED &&
                 p.get_condition() == explore_race->likesbest)) {
              /*  explorations have found an island */
              Claims = i;
              p.set_popn(explore_race->number_sexes);
              p.set_owner(i);
              tot_captured = 1;
            } else
              explore(planet, p, p.get_x(), p.get_y(), i);
          }
          allexp |= o; /* all sectors explored for this player */
        }
    }
  }

  if (allexp) planet.expltimer() = 5;

  /* environment nukes a random sector */
  if (planet.conditions(TOXIC) > ENVIR_DAMAGE_TOX) {
    // TODO(jeffbailey): Replace this with getrandom.
    nukex = int_rand(0, (int)planet.Maxx() - 1);
    nukey = int_rand(0, (int)planet.Maxy() - 1);
    auto& p = smap.get(nukex, nukey);
    p.set_condition(SectorType::SEC_WASTED);
    p.set_popn(0);
    p.set_owner(0);
    p.set_troops(0);
  }

  for (i = 1; i <= Num_races; i++) {
    planet.info(i - 1).prod_crystals = prod_crystals[i - 1];
    planet.info(i - 1).prod_res = prod_res[i - 1];
    planet.info(i - 1).prod_fuel = prod_fuel[i - 1];
    planet.info(i - 1).prod_dest = prod_destruct[i - 1];
    if (planet.info(i - 1).autorep) {
      planet.info(i - 1).autorep--;
      std::stringstream telegram_buf;
      telegram_buf << std::format("\nFrom /{}/{}\n", star.get_name(),
                                  star.get_planet_name(planetnum));

      if (stats.Stinfo[starnum][planetnum].temp_add) {
        telegram_buf << std::format("Temp: {} to {}\n",
                                    planet.conditions(RTEMP),
                                    planet.conditions(TEMP));
      }
      telegram_buf << std::format("Total      Prod: {}r {}f {}d\n",
                                  prod_res[i - 1], prod_fuel[i - 1],
                                  prod_destruct[i - 1]);
      if (prod_crystals[i - 1]) {
        telegram_buf << std::format("    {} crystals found\n",
                                    prod_crystals[i - 1]);
      }
      if (tot_captured) {
        telegram_buf << std::format("{} sectors captured\n", tot_captured);
      }
      if (star.nova_stage()) {
        telegram_buf << std::format(
            "This planet's primary is in a Stage {} nova.\n",
            star.nova_stage());
      }
      /* remind the player that he should clean up the environment. */
      if (planet.conditions(TOXIC) > ENVIR_DAMAGE_TOX) {
        telegram_buf << std::format("Environmental damage on sector {},{}\n",
                                    nukex, nukey);
      }
      if (planet.slaved_to()) {
        telegram_buf << std::format("ENSLAVED to player {}\n",
                                    planet.slaved_to());
      }
      push_telegram(i, star.governor(i - 1), telegram_buf.str());
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
      for (i = 1; i <= Num_races; i++) {
        if (planet.info(i - 1).numsectsowned) {
          push_telegram(i, star.governor(i - 1), telegram_buf.str());
        }
      }
    }
  }

  do_recover(entity_manager, star, planet);

  planet.popn() = 0;
  planet.troops() = 0;
  planet.maxpopn() = 0;
  planet.total_resources() = 0;

  for (i = 1; i <= Num_races; i++) {
    planet.info(i - 1).numsectsowned = 0;
    planet.info(i - 1).popn = 0;
    planet.info(i - 1).troops = 0;
  }

  for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
    Sector& p = sector_wrap;
    if (p.get_owner()) {
      planet.info(p.get_owner() - 1).numsectsowned++;
      planet.info(p.get_owner() - 1).troops += p.get_troops();
      planet.info(p.get_owner() - 1).popn += p.get_popn();
      planet.popn() += p.get_popn();
      planet.troops() += p.get_troops();
      const auto* owner_race = entity_manager.peek_race(p.get_owner());
      planet.maxpopn() += maxsupport(*owner_race, p, Compat[p.get_owner() - 1],
                                     planet.conditions(TOXIC));
      stats.Power[p.get_owner() - 1].troops += p.get_troops();
      stats.Power[p.get_owner() - 1].popn += p.get_popn();
      stats.Power[p.get_owner() - 1].sum_eff += p.get_eff();
      stats.Power[p.get_owner() - 1].sum_mob += p.get_mobilization();
      stats.starpopns[starnum][p.get_owner() - 1] += p.get_popn();
    } else {
      p.set_popn(0);
      p.set_troops(0);
    }
    planet.total_resources() += p.get_resource();
  }

  /* deal with enslaved planets */
  if (planet.slaved_to()) {
    if (planet.info(planet.slaved_to() - 1).popn > planet.popn() / 1000) {
      for (i = 1; i <= Num_races; i++)
        /* add production to slave holder of planet */
        if (planet.info(i - 1).numsectsowned) {
          planet.info(planet.slaved_to() - 1).resource += prod_res[i - 1];
          prod_res[i - 1] = 0;
          planet.info(planet.slaved_to() - 1).fuel += prod_fuel[i - 1];
          prod_fuel[i - 1] = 0;
          planet.info(planet.slaved_to() - 1).destruct += prod_destruct[i - 1];
          prod_destruct[i - 1] = 0;
        }
    } else {
      /* slave revolt! */
      /* first nuke some random sectors from the revolt */
      i = planet.popn() / 1000 + 1;
      while (--i) {
        auto& p = smap.get(int_rand(0, (int)planet.Maxx() - 1),
                           int_rand(0, (int)planet.Maxy() - 1));
        if (p.get_popn() + p.get_troops()) {
          p.set_owner(0);
          p.set_popn(0);
          p.set_troops(0);
          p.set_condition(SectorType::SEC_WASTED);
        }
      }
      /* now nuke all sectors belonging to former master */
      for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
        Sector& p = sector_wrap;
        if (stats.Stinfo[starnum][planetnum].intimidated && random() & 01) {
          if (p.get_owner() == planet.slaved_to()) {
            p.set_owner(0);
            p.set_popn(0);
            p.set_troops(0);
            p.set_condition(SectorType::SEC_WASTED);
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
        for (i = 1; i <= Num_races; i++) {
          if (planet.info(i - 1).numsectsowned) {
            push_telegram(i, star.governor(i - 1), telegram_buf.str());
          }
        }
      }
      planet.slaved_to() = 0;
    }
  }

  /* add production to all people here */
  for (auto race_handle : RaceList(entity_manager)) {
    auto& race = *race_handle;
    player_t player = race.Playernum;
    if (planet.info(player - 1).numsectsowned) {
      planet.info(player - 1).fuel += prod_fuel[player - 1];
      planet.info(player - 1).resource += prod_res[player - 1];
      planet.info(player - 1).destruct += prod_destruct[player - 1];
      planet.info(player - 1).crystals += prod_crystals[player - 1];

      auto gov_idx = star.governor(player - 1);

      /* tax the population - set new tax rate when done */
      if (race.Gov_ship) {
        planet.info(player - 1).prod_money =
            round_rand(INCOME_FACTOR * (double)planet.info(player - 1).tax *
                       (double)planet.info(player - 1).popn);
        race.governor[gov_idx].money += planet.info(player - 1).prod_money;
        planet.info(player - 1).tax +=
            std::min((int)planet.info(player - 1).newtax -
                         (int)planet.info(player - 1).tax,
                     5);
      } else
        planet.info(player - 1).prod_money = 0;
      race.governor[gov_idx].income += planet.info(player - 1).prod_money;

      /* do tech investments */
      if (race.Gov_ship) {
        if (race.governor[gov_idx].money >=
            planet.info(player - 1).tech_invest) {
          planet.info(player - 1).prod_tech =
              tech_prod((int)(planet.info(player - 1).tech_invest),
                        (int)(planet.info(player - 1).popn));
          race.governor[gov_idx].money -= planet.info(player - 1).tech_invest;
          race.tech += planet.info(player - 1).prod_tech;
          race.governor[gov_idx].cost_tech +=
              planet.info(player - 1).tech_invest;
        } else
          planet.info(player - 1).prod_tech = 0;
      } else
        planet.info(player - 1).prod_tech = 0;

      /* build wc's if it's been ordered */
      if (planet.info(player - 1).tox_thresh > 0 &&
          planet.conditions(TOXIC) >= planet.info(player - 1).tox_thresh &&
          planet.info(player - 1).resource >=
              Shipcost(ShipType::OTYPE_TOXWC, race)) {
        ++Num_ships;
        ships =
            (Ship**)realloc(ships, (unsigned)((Num_ships + 1) * sizeof(Ship*)));

        int t = std::min(TOXMAX, planet.conditions(TOXIC));
        planet.conditions(TOXIC) -= t;

        ship_struct data{
            .number = Num_ships,
            .owner = player,
            .governor = star.governor(player - 1),
            .name = std::format("Scum{:04d}", Num_ships),
            .xpos = star.xpos() + planet.xpos(),
            .ypos = star.ypos() + planet.ypos(),
            .mass = 1.0,
            .land_x =
                static_cast<unsigned char>(int_rand(0, (int)planet.Maxx() - 1)),
            .land_y =
                static_cast<unsigned char>(int_rand(0, (int)planet.Maxy() - 1)),
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

        Ship* s2 = new Ship(std::move(data));
        s2->size() = ship_size(*s2);
        ships[Num_ships] = s2;

        insert_sh_plan(planet, s2);
      }
    }
  } /* (if numsectsowned) */

  if (planet.maxpopn() > 0 && planet.conditions(TOXIC) < 100)
    planet.conditions(TOXIC) += planet.popn() / planet.maxpopn();

  if (planet.conditions(TOXIC) > 100)
    planet.conditions(TOXIC) = 100;
  else if (planet.conditions(TOXIC) < 0)
    planet.conditions(TOXIC) = 0;

  for (i = 1; i <= Num_races; i++) {
    stats.Power[i - 1].resource += planet.info(i - 1).resource;
    stats.Power[i - 1].destruct += planet.info(i - 1).destruct;
    stats.Power[i - 1].fuel += planet.info(i - 1).fuel;
    stats.Power[i - 1].sectors_owned += planet.info(i - 1).numsectsowned;
    stats.Power[i - 1].planets_owned += !!planet.info(i - 1).numsectsowned;
    if (planet.info(i - 1).numsectsowned) {
      /* combat readiness naturally moves towards the avg mobilization */
      planet.info(i - 1).mob_points = avg_mob[i - 1];
      avg_mob[i - 1] /= (int)planet.info(i - 1).numsectsowned;
      planet.info(i - 1).comread = avg_mob[i - 1];
    } else
      planet.info(i - 1).comread = 0;
    planet.info(i - 1).guns = planet_guns(planet.info(i - 1).mob_points);
  }
  putsmap(smap, planet);
  return allmod;
}
