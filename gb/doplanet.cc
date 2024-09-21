// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  doplanet.c -- do one turn on a planet. */

import gblib;
import std.compat;

#include "gb/doplanet.h"

#include <strings.h>

#include <cstdlib>

#include "gb/bombard.h"

namespace {
bool moveship_onplanet(Ship &ship, const Planet &planet) {
  if (ship.shipclass[ship.special.terraform.index] == 's') {
    ship.on = 0;
    return false;
  }
  if (ship.shipclass[ship.special.terraform.index] == 'c')
    ship.special.terraform.index = 0; /* reset the orders */

  auto [x, y] = get_move(planet, ship.shipclass[ship.special.terraform.index],
                         {ship.land_x, ship.land_y});

  bool bounced = false;

  if (y >= planet.Maxy) {
    bounced = true;
    y -= 2; /* bounce off of south pole! */
  } else if (y < 0)
    y = 1;
  bounced = true; /* bounce off of north pole! */
  if (planet.Maxy == 1) y = 0;
  if (ship.shipclass[ship.special.terraform.index + 1] != '\0') {
    ++ship.special.terraform.index;
    if ((ship.shipclass[ship.special.terraform.index + 1] == '\0') &&
        (!ship.notified)) {
      ship.notified = 1;
      std::string teleg_buf =
          std::format("%{0} is out of orders at %{1}.", ship_to_string(ship),
                      prin_ship_orbits(ship));
      push_telegram(ship.owner, ship.governor, teleg_buf);
    }
  } else if (bounced)
    ship.shipclass[ship.special.terraform.index] +=
        ((ship.shipclass[ship.special.terraform.index] > '5') ? -6 : 6);
  ship.land_x = x;
  ship.land_y = y;
  return true;
}

// move, and then terraform
void terraform(Ship &ship, Planet &planet, SectorMap &smap) {
  if (!moveship_onplanet(ship, planet)) return;
  auto &s = smap.get(ship.land_x, ship.land_y);

  if (s.condition == races[ship.owner - 1].likesbest) {
    std::string buf = std::format(" T{} is full of zealots!!!", ship.number);
    push_telegram(ship.owner, ship.governor, buf);
    return;
  }

  if (s.condition == SectorType::SEC_GAS) {
    std::string buf =
        std::format(" T{} is trying to terraform gas.", ship.number);
    push_telegram(ship.owner, ship.governor, buf);
    return;
  }

  if (success((100 - (int)ship.damage) * ship.popn / ship.max_crew)) {
    /* only condition can be terraformed, type doesn't change */
    s.condition = races[ship.owner - 1].likesbest;
    s.eff = 0;
    s.mobilization = 0;
    s.popn = 0;
    s.troops = 0;
    s.owner = 0;
    use_fuel(ship, FUEL_COST_TERRA);
    if ((random() & 01) && (planet.conditions[TOXIC] < 100))
      planet.conditions[TOXIC] += 1;
    if ((ship.fuel < (double)FUEL_COST_TERRA) && (!ship.notified)) {
      ship.notified = 1;
      msg_OOF(ship);
    }
  }
}

void plow(Ship *ship, Planet &planet, SectorMap &smap) {
  if (!moveship_onplanet(*ship, planet)) return;
  auto &s = smap.get(ship->land_x, ship->land_y);
  if ((races[ship->owner - 1].likes[s.condition]) && (s.fert < 100)) {
    int adjust = round_rand(
        10 * (0.01 * (100.0 - (double)ship->damage) * (double)ship->popn) /
        ship->max_crew);
    if ((ship->fuel < (double)FUEL_COST_PLOW) && (!ship->notified)) {
      ship->notified = 1;
      msg_OOF(*ship);
      return;
    }
    s.fert = std::min(100U, s.fert + adjust);
    if (s.fert >= 100) {
      std::string buf = std::format(" K{} is full of zealots!!!", ship->number);
      push_telegram(ship->owner, ship->governor, buf);
    }
    use_fuel(*ship, FUEL_COST_PLOW);
    if ((random() & 01) && (planet.conditions[TOXIC] < 100))
      planet.conditions[TOXIC] += 1;
  }
}

void do_dome(Ship *ship, SectorMap &smap) {
  int adjust;

  auto &s = smap.get(ship->land_x, ship->land_y);
  if (s.eff >= 100) {
    std::string buf = std::format(" Y{} is full of zealots!!!", ship->number);
    push_telegram(ship->owner, ship->governor, buf);
    return;
  }
  adjust = round_rand(.05 * (100. - (double)ship->damage) * (double)ship->popn /
                      ship->max_crew);
  s.eff += adjust;
  s.eff = std::min<unsigned int>(s.eff, 100);
  use_resource(*ship, RES_COST_DOME);
}

void do_quarry(Ship *ship, Planet &planet, SectorMap &smap) {
  int prod;
  int tox;

  auto &s = smap.get(ship->land_x, ship->land_y);

  if ((ship->fuel < (double)FUEL_COST_QUARRY)) {
    if (!ship->notified) msg_OOF(*ship);
    ship->notified = 1;
    return;
  }
  /* nuke the sector */
  s.condition = SectorType::SEC_WASTED;
  prod = round_rand(races[ship->owner - 1].metabolism * (double)ship->popn /
                    (double)ship->max_crew);
  ship->fuel -= FUEL_COST_QUARRY;
  prod_res[ship->owner - 1] += prod;
  tox = int_rand(0, int_rand(0, prod));
  planet.conditions[TOXIC] = std::min(100, planet.conditions[TOXIC] + tox);
  if (s.fert >= prod)
    s.fert -= prod;
  else
    s.fert = 0;
}

void do_berserker(Ship *ship, Planet &planet) {
  if (ship->whatdest == ScopeLevel::LEVEL_PLAN &&
      ship->whatorbits == ScopeLevel::LEVEL_PLAN && !landed(*ship) &&
      ship->storbits == ship->deststar && ship->pnumorbits == ship->destpnum) {
    if (!berserker_bombard(*ship, planet, races[ship->owner - 1]))
      ship->destpnum = int_rand(0, stars[ship->storbits].numplanets - 1);
    else if (Sdata.VN_hitlist[ship->special.mind.who_killed - 1] > 0)
      --Sdata.VN_hitlist[ship->special.mind.who_killed - 1];
  }
}

void do_recover(Planet &planet, int starnum, int planetnum) {
  int owners = 0;
  player_t i;
  player_t j;
  int stolenres = 0;
  int stolendes = 0;
  int stolenfuel = 0;
  int stolencrystals = 0;
  int all_buddies_here = 1;

  uint64_t ownerbits = 0;

  for (i = 1; i <= Num_races && all_buddies_here; i++) {
    if (planet.info[i - 1].numsectsowned > 0) {
      owners++;
      setbit(ownerbits, i);
      for (j = 1; j < i && all_buddies_here; j++)
        if (isset(ownerbits, j) &&
            (!isset(races[i - 1].allied, j) || !isset(races[j - 1].allied, i)))
          all_buddies_here = 0;
    } else {        /* Player i owns no sectors */
      if (i != 1) { /* Can't steal from God */
        stolenres += planet.info[i - 1].resource;
        stolendes += planet.info[i - 1].destruct;
        stolenfuel += planet.info[i - 1].fuel;
        stolencrystals += planet.info[i - 1].crystals;
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
                                    stars[starnum].name,
                                    stars[starnum].pnames[planetnum]);
        push_telegram(i, stars[starnum].governor[i - 1], telegram_buf.str());
        telegram_buf.str("");
        telegram_buf << std::format("{:<14} {:>5} {:>5} {:>5} {:>5}\n", "",
                                    "res", "destr", "fuel", "xtal");
        push_telegram(i, stars[starnum].governor[i - 1], telegram_buf.str());
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
        planet.info[i - 1].resource += res;
        givenres += res;
        planet.info[i - 1].destruct += des;
        givendes += des;
        planet.info[i - 1].fuel += fuel;
        givenfuel += fuel;
        planet.info[i - 1].crystals += crystals;
        givencrystals += crystals;

        owners--;
        {
          std::stringstream telegram_buf;
          telegram_buf << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}",
                                      races[i - 1].name, res, des, fuel,
                                      crystals);
          for (j = 1; j <= Num_races; j++) {
            if (isset(ownerbits, j)) {
              push_telegram(j, stars[starnum].governor[j - 1],
                            telegram_buf.str());
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

      planet.info[i - 1].resource += res;
      planet.info[i - 1].destruct += des;
      planet.info[i - 1].fuel += fuel;
      planet.info[i - 1].crystals += crystals;
      {
        std::stringstream first_telegram;
        first_telegram << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}",
                                      races[i - 1].name, res, des, fuel,
                                      crystals);
        std::stringstream second_telegram;
        second_telegram << std::format("{:<14.14s} {:>5} {:>5} {:>5} {:>5}\n",
                                       "Total:", stolenres, stolendes,
                                       stolenfuel, stolencrystals);
        for (j = 1; j <= Num_races; j++) {
          if (isset(ownerbits, j)) {
            push_telegram(j, stars[starnum].governor[j - 1],
                          first_telegram.str());
            push_telegram(j, stars[starnum].governor[j - 1],
                          second_telegram.str());
          }
        }
      }
    } else {
      push_telegram(1, 0, "Bug in stealing resources\n");
    }
    /* Next: take all the loot away from the losers */
    for (i = 2; i <= Num_races; i++)
      if (!isset(ownerbits, i)) {
        planet.info[i - 1].resource = 0;
        planet.info[i - 1].destruct = 0;
        planet.info[i - 1].fuel = 0;
        planet.info[i - 1].crystals = 0;
      }
  }
}

double est_production(const Sector &s) {
  return (races[s.owner - 1].metabolism * (double)s.eff * (double)s.eff /
          200.0);
}
}  // namespace

int doplanet(const int starnum, Planet &planet, const int planetnum) {
  int shipno;
  int nukex;
  int nukey;
  int o = 0;
  int i;
  Ship *ship;
  double fadd;
  int timer = 20;
  unsigned char allmod = 0;
  unsigned char allexp = 0;

  bzero((char *)Sectinfo, sizeof(Sectinfo));

  bzero((char *)avg_mob, sizeof(avg_mob));
  bzero((char *)prod_res, sizeof(prod_res));
  bzero((char *)prod_fuel, sizeof(prod_fuel));
  bzero((char *)prod_destruct, sizeof(prod_destruct));
  bzero((char *)prod_crystals, sizeof(prod_crystals));

  tot_resdep = prod_eff = prod_mob = tot_captured = 0;
  Claims = 0;

  planet.maxpopn = 0;

  planet.popn = 0; /* initialize population for recount */
  planet.troops = 0;
  planet.total_resources = 0;

  /* reset global variables */
  for (i = 1; i <= Num_races; i++) {
    Compat[i - 1] = planet.compatibility(races[i - 1]);
    planet.info[i - 1].numsectsowned = 0;
    planet.info[i - 1].troops = 0;
    planet.info[i - 1].popn = 0;
    planet.info[i - 1].est_production = 0.0;
    prod_crystals[i - 1] = 0;
    prod_fuel[i - 1] = 0;
    prod_destruct[i - 1] = 0;
    prod_res[i - 1] = 0;
    avg_mob[i - 1] = 0;
  }

  auto smap = getsmap(planet);
  shipno = planet.ships;
  while (shipno) {
    ship = ships[shipno];
    if (ship->alive && !ship->rad) {
      /* planet level functions - do these here because they use the sector map
              or affect planet production */
      switch (ship->type) {
        case ShipType::OTYPE_VN:
          planet_doVN(*ship, planet, smap);
          break;
        case ShipType::OTYPE_BERS:
          if (!ship->destruct || !ship->bombard)
            planet_doVN(*ship, planet, smap);
          else
            do_berserker(ship, planet);
          break;
        case ShipType::OTYPE_TERRA:
          if ((ship->on && landed(*ship) && ship->popn)) {
            if (ship->fuel >= (double)FUEL_COST_TERRA)
              terraform(*ship, planet, smap);
            else if (!ship->notified) {
              ship->notified = 1;
              msg_OOF(*ship);
            }
          }
          break;
        case ShipType::OTYPE_PLOW:
          if (ship->on && landed(*ship)) {
            if (ship->fuel >= (double)FUEL_COST_PLOW)
              plow(ship, planet, smap);
            else if (!ship->notified) {
              ship->notified = 1;
              msg_OOF(*ship);
            }
          } else if (ship->on) {
            std::string buf = std::format("K{} is not landed.", ship->number);
            push_telegram(ship->owner, ship->governor, buf);
          } else {
            std::string buf =
                std::format("K{} is not switched on.", ship->number);
            push_telegram(ship->owner, ship->governor, buf);
          }
          break;
        case ShipType::OTYPE_DOME:
          if (ship->on && landed(*ship)) {
            if (ship->resource >= RES_COST_DOME)
              do_dome(ship, smap);
            else {
              std::string buf = std::format(
                  "Y{} does not have enough resources.", ship->number);
              push_telegram(ship->owner, ship->governor, buf);
            }
          } else if (ship->on) {
            std::string buf = std::format("Y{} is not landed.", ship->number);
            push_telegram(ship->owner, ship->governor, buf);
          } else {
            std::string buf =
                std::format("Y{} is not switched on.", ship->number);
            push_telegram(ship->owner, ship->governor, buf);
          }
          break;
        case ShipType::OTYPE_WPLANT:
          if (landed(*ship))
            if (ship->resource >= RES_COST_WPLANT &&
                ship->fuel >= FUEL_COST_WPLANT)
              prod_destruct[ship->owner - 1] += do_weapon_plant(*ship);
            else {
              if (ship->resource < RES_COST_WPLANT) {
                std::string buf = std::format(
                    "W{} does not have enough resources.", ship->number);
                push_telegram(ship->owner, ship->governor, buf);
              } else {
                std::string buf =
                    std::format("W{} does not have enough fuel.", ship->number);
                push_telegram(ship->owner, ship->governor, buf);
              }
            }
          else {
            std::string buf = std::format("W{} is not landed.", ship->number);
            push_telegram(ship->owner, ship->governor, buf);
          }
          break;
        case ShipType::OTYPE_QUARRY:
          if ((ship->on && landed(*ship) && ship->popn)) {
            if (ship->fuel >= FUEL_COST_QUARRY)
              do_quarry(ship, planet, smap);
            else if (!ship->notified) {
              ship->on = 0;
              msg_OOF(*ship);
            }
          } else {
            std::string buf;
            if (!ship->on) {
              buf = std::format("q{} is not switched on.", ship->number);
            }
            if (!landed(*ship)) {
              buf = std::format("q{} is not landed.", ship->number);
            }
            if (!ship->popn) {
              buf = std::format("q{} does not have workers aboard.",
                                ship->number);
            }
            push_telegram(ship->owner, ship->governor, buf);
          }
          break;
        default:
          break;
      }
      /* add fuel for ships orbiting a gas giant */
      if (!landed(*ship) && planet.type == PlanetType::GASGIANT) {
        switch (ship->type) {
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
        fadd = std::min((double)max_fuel(*ship) - ship->fuel, fadd);
        rcv_fuel(*ship, fadd);
      }
    }
    shipno = ship->nextship;
  }

  /* check for space mirrors (among other things) warming the planet */
  /* if a change in any artificial warming/cooling trends */
  planet.conditions[TEMP] = planet.conditions[RTEMP] +
                            Stinfo[starnum][planetnum].temp_add +
                            int_rand(-5, 5);

  for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
    Sector &p = sector_wrap;
    if (p.owner && (p.popn || p.troops)) {
      allmod = 1;
      if (!stars[starnum].nova_stage) {
        produce(stars[starnum], planet, p);
        if (p.owner)
          planet.info[p.owner - 1].est_production += est_production(p);
        spread(planet, p, smap);
      } else {
        /* damage sector from supernova */
        p.resource++;
        p.fert *= 0.8;
        if (stars[starnum].nova_stage == 14)
          p.popn = p.owner = p.troops = 0;
        else
          p.popn = round_rand((double)p.popn * .50);
      }
      Sectinfo[p.x][p.y].done = 1;
    }

    if ((!p.popn && !p.troops) || !p.owner) {
      p.owner = 0;
      p.popn = p.troops = 0;
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

  for (auto &p : smap) {
    if (p.owner) planet.info[p.owner - 1].numsectsowned++;
  }

  if (planet.expltimer >= 1) planet.expltimer--;
  if (!stars[starnum].nova_stage && !planet.expltimer) {
    if (!planet.expltimer) planet.expltimer = 5;
    for (i = 1; !Claims && !allexp && i <= Num_races; i++) {
      /* sectors have been modified for this player*/
      if (planet.info[i - 1].numsectsowned > 0)
        while (!Claims && !allexp && timer > 0) {
          timer -= 1;
          o = 1;
          for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
            if (!Claims) break;
            Sector &p = sector_wrap;
            /* find out if all sectors have been explored */
            o &= Sectinfo[p.x][p.y].explored;
            if (((Sectinfo[p.x][p.y].explored == i) && !(random() & 02)) &&
                (!p.owner && p.condition != SectorType::SEC_WASTED &&
                 p.condition == races[i - 1].likesbest)) {
              /*  explorations have found an island */
              Claims = i;
              p.popn = races[i - 1].number_sexes;
              p.owner = i;
              tot_captured = 1;
            } else
              explore(planet, p, p.x, p.y, i);
          }
          allexp |= o; /* all sectors explored for this player */
        }
    }
  }

  if (allexp) planet.expltimer = 5;

  /* environment nukes a random sector */
  if (planet.conditions[TOXIC] > ENVIR_DAMAGE_TOX) {
    // TODO(jeffbailey): Replace this with getrandom.
    nukex = int_rand(0, (int)planet.Maxx - 1);
    nukey = int_rand(0, (int)planet.Maxy - 1);
    auto &p = smap.get(nukex, nukey);
    p.condition = SectorType::SEC_WASTED;
    p.popn = p.owner = p.troops = 0;
  }

  for (i = 1; i <= Num_races; i++) {
    planet.info[i - 1].prod_crystals = prod_crystals[i - 1];
    planet.info[i - 1].prod_res = prod_res[i - 1];
    planet.info[i - 1].prod_fuel = prod_fuel[i - 1];
    planet.info[i - 1].prod_dest = prod_destruct[i - 1];
    if (planet.info[i - 1].autorep) {
      planet.info[i - 1].autorep--;
      std::stringstream telegram_buf;
      telegram_buf << std::format("\nFrom /{}/{}\n", stars[starnum].name,
                                  stars[starnum].pnames[planetnum]);

      if (Stinfo[starnum][planetnum].temp_add) {
        telegram_buf << std::format("Temp: {} to {}\n",
                                    planet.conditions[RTEMP],
                                    planet.conditions[TEMP]);
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
      if (stars[starnum].nova_stage) {
        telegram_buf << std::format(
            "This planet's primary is in a Stage {} nova.\n",
            stars[starnum].nova_stage);
      }
      /* remind the player that he should clean up the environment. */
      if (planet.conditions[TOXIC] > ENVIR_DAMAGE_TOX) {
        telegram_buf << std::format("Environmental damage on sector {},{}\n",
                                    nukex, nukey);
      }
      if (planet.slaved_to) {
        telegram_buf << std::format("ENSLAVED to player {}\n",
                                    planet.slaved_to);
      }
      push_telegram(i, stars[starnum].governor[i - 1], telegram_buf.str());
    }
  }

  /* find out who is on this planet, for nova notification */
  if (stars[starnum].nova_stage == 1) {
    {
      std::stringstream telegram_buf;
      telegram_buf << std::format("BULLETIN from /{}/{}\n", stars[starnum].name,
                                  stars[starnum].pnames[planetnum]);
      telegram_buf << std::format("\nStar {} is undergoing nova.\n",
                                  stars[starnum].name);
      if (planet.type == PlanetType::EARTH ||
          planet.type == PlanetType::WATER ||
          planet.type == PlanetType::FOREST) {
        telegram_buf << "Seas and rivers are boiling!\n";
      }
      telegram_buf << "This planet must be evacuated immediately!\n"
                   << TELEG_DELIM;
      for (i = 1; i <= Num_races; i++) {
        if (planet.info[i - 1].numsectsowned) {
          push_telegram(i, stars[starnum].governor[i - 1], telegram_buf.str());
        }
      }
    }
  }

  do_recover(planet, starnum, planetnum);

  planet.popn = 0;
  planet.troops = 0;
  planet.maxpopn = 0;
  planet.total_resources = 0;

  for (i = 1; i <= Num_races; i++) {
    planet.info[i - 1].numsectsowned = 0;
    planet.info[i - 1].popn = 0;
    planet.info[i - 1].troops = 0;
  }

  for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
    Sector &p = sector_wrap;
    if (p.owner) {
      planet.info[p.owner - 1].numsectsowned++;
      planet.info[p.owner - 1].troops += p.troops;
      planet.info[p.owner - 1].popn += p.popn;
      planet.popn += p.popn;
      planet.troops += p.troops;
      planet.maxpopn += maxsupport(races[p.owner - 1], p, Compat[p.owner - 1],
                                   planet.conditions[TOXIC]);
      Power[p.owner - 1].troops += p.troops;
      Power[p.owner - 1].popn += p.popn;
      Power[p.owner - 1].sum_eff += p.eff;
      Power[p.owner - 1].sum_mob += p.mobilization;
      starpopns[starnum][p.owner - 1] += p.popn;
    } else {
      p.popn = 0;
      p.troops = 0;
    }
    planet.total_resources += p.resource;
  }

  /* deal with enslaved planets */
  if (planet.slaved_to) {
    if (planet.info[planet.slaved_to - 1].popn > planet.popn / 1000) {
      for (i = 1; i <= Num_races; i++)
        /* add production to slave holder of planet */
        if (planet.info[i - 1].numsectsowned) {
          planet.info[planet.slaved_to - 1].resource += prod_res[i - 1];
          prod_res[i - 1] = 0;
          planet.info[planet.slaved_to - 1].fuel += prod_fuel[i - 1];
          prod_fuel[i - 1] = 0;
          planet.info[planet.slaved_to - 1].destruct += prod_destruct[i - 1];
          prod_destruct[i - 1] = 0;
        }
    } else {
      /* slave revolt! */
      /* first nuke some random sectors from the revolt */
      i = planet.popn / 1000 + 1;
      while (--i) {
        auto &p = smap.get(int_rand(0, (int)planet.Maxx - 1),
                           int_rand(0, (int)planet.Maxy - 1));
        if (p.popn + p.troops) {
          p.owner = p.popn = p.troops = 0;
          p.condition = SectorType::SEC_WASTED;
        }
      }
      /* now nuke all sectors belonging to former master */
      for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
        Sector &p = sector_wrap;
        if (Stinfo[starnum][planetnum].intimidated && random() & 01) {
          if (p.owner == planet.slaved_to) {
            p.owner = 0;
            p.popn = 0;
            p.troops = 0;
            p.condition = SectorType::SEC_WASTED;
          }
        }
        /* also add up the populations while here */
      }
      {
        std::stringstream telegram_buf;
        telegram_buf << std::format(
            "\nThere has been a SLAVE REVOLT on /{}/{}!\n", stars[starnum].name,
            stars[starnum].pnames[planetnum]);
        telegram_buf << std::format(
            "All population belonging to player #{} on the planet have been "
            "killed!\n",
            planet.slaved_to);
        telegram_buf << "Productions now go to their rightful owners.\n";
        for (i = 1; i <= Num_races; i++) {
          if (planet.info[i - 1].numsectsowned) {
            push_telegram(i, stars[starnum].governor[i - 1],
                          telegram_buf.str());
          }
        }
      }
      planet.slaved_to = 0;
    }
  }

  /* add production to all people here */
  for (i = 1; i <= Num_races; i++)
    if (planet.info[i - 1].numsectsowned) {
      planet.info[i - 1].fuel += prod_fuel[i - 1];
      planet.info[i - 1].resource += prod_res[i - 1];
      planet.info[i - 1].destruct += prod_destruct[i - 1];
      planet.info[i - 1].crystals += prod_crystals[i - 1];

      /* tax the population - set new tax rate when done */
      if (races[i - 1].Gov_ship) {
        planet.info[i - 1].prod_money =
            round_rand(INCOME_FACTOR * (double)planet.info[i - 1].tax *
                       (double)planet.info[i - 1].popn);
        races[i - 1].governor[stars[starnum].governor[i - 1]].money +=
            planet.info[i - 1].prod_money;
        planet.info[i - 1].tax += std::min(
            (int)planet.info[i - 1].newtax - (int)planet.info[i - 1].tax, 5);
      } else
        planet.info[i - 1].prod_money = 0;
      races[i - 1].governor[stars[starnum].governor[i - 1]].income +=
          planet.info[i - 1].prod_money;

      /* do tech investments */
      if (races[i - 1].Gov_ship) {
        if (races[i - 1].governor[stars[starnum].governor[i - 1]].money >=
            planet.info[i - 1].tech_invest) {
          planet.info[i - 1].prod_tech =
              tech_prod((int)(planet.info[i - 1].tech_invest),
                        (int)(planet.info[i - 1].popn));
          races[i - 1].governor[stars[starnum].governor[i - 1]].money -=
              planet.info[i - 1].tech_invest;
          races[i - 1].tech += planet.info[i - 1].prod_tech;
          races[i - 1].governor[stars[starnum].governor[i - 1]].cost_tech +=
              planet.info[i - 1].tech_invest;
        } else
          planet.info[i - 1].prod_tech = 0;
      } else
        planet.info[i - 1].prod_tech = 0;

      /* build wc's if it's been ordered */
      if (planet.info[i - 1].tox_thresh > 0 &&
          planet.conditions[TOXIC] >= planet.info[i - 1].tox_thresh &&
          planet.info[i - 1].resource >=
              Shipcost(ShipType::OTYPE_TOXWC, races[i - 1])) {
        Ship *s2;
        int t;
        ++Num_ships;
        ships = (Ship **)realloc(ships,
                                 (unsigned)((Num_ships + 1) * sizeof(Ship *)));
        s2 = ships[Num_ships] = (Ship *)malloc(sizeof(Ship));
        bzero((char *)s2, sizeof(Ship));
        s2->number = Num_ships;
        s2->type = ShipType::OTYPE_TOXWC;

        s2->armor = Shipdata[ShipType::OTYPE_TOXWC][ABIL_ARMOR];
        s2->guns = GTYPE_NONE;
        s2->primary = Shipdata[ShipType::OTYPE_TOXWC][ABIL_GUNS];
        s2->primtype = Shipdata[ShipType::OTYPE_TOXWC][ABIL_PRIMARY];
        s2->secondary = Shipdata[ShipType::OTYPE_TOXWC][ABIL_GUNS];
        s2->sectype = Shipdata[ShipType::OTYPE_TOXWC][ABIL_SECONDARY];
        s2->max_crew = Shipdata[ShipType::OTYPE_TOXWC][ABIL_MAXCREW];
        s2->max_resource = Shipdata[ShipType::OTYPE_TOXWC][ABIL_CARGO];
        s2->max_fuel = Shipdata[ShipType::OTYPE_TOXWC][ABIL_FUELCAP];
        s2->max_destruct = Shipdata[ShipType::OTYPE_TOXWC][ABIL_DESTCAP];
        s2->max_speed = Shipdata[ShipType::OTYPE_TOXWC][ABIL_SPEED];
        s2->build_cost = Shipcost(ShipType::OTYPE_TOXWC, races[i - 1]);
        s2->size = ship_size(*s2);
        s2->base_mass = 1.0; /* a hack */
        s2->mass = s2->base_mass;
        s2->alive = 1;
        s2->active = 1;
        sprintf(s2->name, "Scum%04ld", Num_ships);

        insert_sh_plan(planet, s2);

        s2->whatorbits = ScopeLevel::LEVEL_PLAN;
        s2->storbits = starnum;
        s2->pnumorbits = planetnum;
        s2->docked = 1;
        s2->xpos = stars[starnum].xpos + planet.xpos;
        s2->ypos = stars[starnum].ypos + planet.ypos;
        s2->land_x = int_rand(0, (int)planet.Maxx - 1);
        s2->land_y = int_rand(0, (int)planet.Maxy - 1);
        s2->whatdest = ScopeLevel::LEVEL_PLAN;
        s2->deststar = starnum;
        s2->destpnum = planetnum;
        s2->owner = i;
        s2->governor = stars[starnum].governor[i - 1];
        t = std::min(TOXMAX, planet.conditions[TOXIC]); /* amt of tox */
        planet.conditions[TOXIC] -= t;
        s2->special.waste.toxic = t;
      }
    } /* (if numsectsowned[i]) */

  if (planet.maxpopn > 0 && planet.conditions[TOXIC] < 100)
    planet.conditions[TOXIC] += planet.popn / planet.maxpopn;

  if (planet.conditions[TOXIC] > 100)
    planet.conditions[TOXIC] = 100;
  else if (planet.conditions[TOXIC] < 0)
    planet.conditions[TOXIC] = 0;

  for (i = 1; i <= Num_races; i++) {
    Power[i - 1].resource += planet.info[i - 1].resource;
    Power[i - 1].destruct += planet.info[i - 1].destruct;
    Power[i - 1].fuel += planet.info[i - 1].fuel;
    Power[i - 1].sectors_owned += planet.info[i - 1].numsectsowned;
    Power[i - 1].planets_owned += !!planet.info[i - 1].numsectsowned;
    if (planet.info[i - 1].numsectsowned) {
      /* combat readiness naturally moves towards the avg mobilization */
      planet.info[i - 1].mob_points = avg_mob[i - 1];
      avg_mob[i - 1] /= (int)planet.info[i - 1].numsectsowned;
      planet.info[i - 1].comread = avg_mob[i - 1];
    } else
      planet.info[i - 1].comread = 0;
    planet.info[i - 1].guns = planet_guns(planet.info[i - 1].mob_points);
  }
  putsmap(smap, planet);
  return allmod;
}
