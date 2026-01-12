// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import dallib;
import std;

#include <strings.h>

#include <cstdio>

#include "gb/racegen.h"

namespace {
constexpr std::array<PlanetType, N_HOME_PLANET_TYPES> planet_translate = {
    PlanetType::EARTH,   PlanetType::FOREST, PlanetType::DESERT,
    PlanetType::WATER,   PlanetType::MARS,   PlanetType::ICEBALL,
    PlanetType::GASGIANT};
}

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race() {
  int star;
  int pnum;
  /*
    if (race.status == STATUS_ENROLLED) {
      sprintf(race.rejection, "This race has already been enrolled!\n") ;
      return 1 ;
      }
  */
  // Create Database and EntityManager for dependency injection
  Database database{PKGSTATEDIR "gb.db"};
  EntityManager entity_manager{database};
  JsonStore store{database};

  auto Playernum = player_t{entity_manager.num_races().value + 1};
  if ((Playernum == player_t{1}) && (race_info.priv_type != P_GOD)) {
    sprintf(race_info.rejection,
            "The first race enrolled must have God privileges.\n");
    return 1;
  }
  if (Playernum >= MAXPLAYERS) {
    sprintf(race_info.rejection,
            "There are already %d players; No more allowed.\n", MAXPLAYERS - 1);
    race_info.status = STATUS_UNENROLLABLE;
    return 1;
  }

  const auto* universe = entity_manager.peek_universe();
  auto numstars = universe->numstars;

  std::cout << std::format("Looking for {}..",
                           planet_print_name[race_info.home_planet_type]);

  auto ppref = planet_translate[race_info.home_planet_type];
  std::array<int, NUMSTARS> indirect;
  std::ranges::iota(indirect, 0);
  auto last_star_left = numstars - 1;
  while (last_star_left >= 0) {
    auto i = int_rand(0, last_star_left);
    star = indirect[i];

    std::cout << ".";

    const auto* star_ptr = entity_manager.peek_star(star);
    // Skip over inhabited stars and stars with few planets. */
    if ((star_ptr->numplanets() < 2) || star_ptr->inhabited()) {
    } else {
      /* look for uninhabited planets */
      for (pnum = 0; pnum < star_ptr->numplanets(); pnum++) {
        const auto* planet_ptr = entity_manager.peek_planet(star, pnum);
        if ((planet_ptr->type() == ppref) &&
            (planet_ptr->conditions(RTEMP) >= -200) &&
            (planet_ptr->conditions(RTEMP) <= 100))
          goto found_planet;
      }
    }
    /*
     * Since we are here, this star didn't work out: */
    indirect[i] = indirect[last_star_left--];
  }

  /*
   * If we get here, then we did not find any good planet. */
  std::cout << " failed!\n";
  sprintf(race_info.rejection,
          "Didn't find any free %s; choose another home planet type.\n",
          planet_print_name[race_info.home_planet_type]);
  race_info.status = STATUS_UNENROLLABLE;
  return 1;

found_planet:
  std::cout << " found!\n";

  // Get handles for modification
  auto planet_handle = entity_manager.get_planet(star, pnum);
  auto& planet = *planet_handle;

  auto race = new Race{};

  race->Playernum = Playernum;
  race->God = (race_info.priv_type == P_GOD);
  race->Guest = (race_info.priv_type == P_GUEST);
  race->name = race_info.name;
  race->password = race_info.password;

  race->governor[0].password = "0";
  race->governor[0].homelevel = race->governor[0].deflevel =
      ScopeLevel::LEVEL_PLAN;
  race->governor[0].homesystem = race->governor[0].defsystem = star;
  race->governor[0].homeplanetnum = race->governor[0].defplanetnum = pnum;
  /* display options */
  race->governor[0].toggle.highlight = Playernum;
  race->governor[0].toggle.inverse = true;
  race->governor[0].toggle.color = false;
  race->governor[0].active = true;

  for (auto i = 0; i <= OTHER; i++)
    race->conditions[i] = planet.conditions(static_cast<Conditions>(i));

  for (auto i = 1; i <= MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == Playernum) || (Playernum == 1) || race->God)
      race->translate[i - 1] = 100; /* you can talk to own race */
    else
      race->translate[i - 1] = 1;
  }

  // Assign racial characteristics
  race->absorb = (race_info.attr[ABSORB] != 0.0);
  race->collective_iq = (race_info.attr[COL_IQ] != 0.0);
  race->Metamorph = (race_info.race_type == R_METAMORPH);
  race->pods = (race_info.attr[PODS] != 0.0);

  race->fighters = race_info.attr[FIGHT];
  if (race_info.attr[COL_IQ] == 1.0)
    race->IQ_limit = race_info.attr[A_IQ];
  else
    race->IQ = race_info.attr[A_IQ];
  race->number_sexes = race_info.attr[SEXES];

  race->fertilize = race_info.attr[FERT] * 100;

  race->adventurism = race_info.attr[ADVENT];
  race->birthrate = race_info.attr[BIRTH];
  race->mass = race_info.attr[MASS];
  race->metabolism = race_info.attr[METAB];

  // Assign sector compats and determine a primary sector type.
  for (auto i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++) {
    race->likes[i] = race_info.compat[i] / 100.0;
    if ((100 == race_info.compat[i]) &&
        (1.0 == planet_compat_cov[race_info.home_planet_type][i]))
      race->likesbest = i;
  }

  // Find sector to build capital on, and populate it
  auto smap_handle = entity_manager.get_sectormap(star, pnum);
  auto& smap = *smap_handle;

  Sector& sect = [&]() -> Sector& {
    for (auto shuffled = smap.shuffle(); const auto& sector_wrap : shuffled) {
      Sector& current_sect = sector_wrap.get();
      if (current_sect.get_condition() == race->likesbest) {
        return current_sect;
      }
    }
    // We default to putting the capital at 0,0 if we don't have a better choice
    return smap.get(0, 0);
  }();

  sect.set_owner(Playernum);
  sect.set_race(Playernum);
  sect.set_popn(race->number_sexes);
  planet.popn() = race->number_sexes;
  sect.set_fert(100);
  sect.set_eff(10);
  sect.set_troops(0);
  planet.troops() = 0;

  race->governors = 0;

  // Build a capital ship to run the government
  {
    ship_struct ss{};  // POD struct for initialization

    auto shipno = entity_manager.num_ships() + 1;
    race->Gov_ship = shipno;
    planet.ships() = shipno;
    ss.nextship = 0;

    ss.type = ShipType::OTYPE_GOV;
    const auto* star_ptr = entity_manager.peek_star(star);
    ss.xpos = star_ptr->xpos() + planet.xpos();
    ss.ypos = star_ptr->ypos() + planet.ypos();
    ss.land_x = sect.get_x();
    ss.land_y = sect.get_y();

    ss.speed = 0;
    ss.owner = Playernum;
    ss.race = Playernum;
    ss.governor = 0;

    ss.tech = 100.0;

    ss.build_type = ShipType::OTYPE_GOV;
    ss.armor = Shipdata[ShipType::OTYPE_GOV][ABIL_ARMOR];
    ss.guns = PRIMARY;
    ss.primary = Shipdata[ShipType::OTYPE_GOV][ABIL_GUNS];
    ss.primtype = shipdata_primary(ShipType::OTYPE_GOV);
    ss.secondary = Shipdata[ShipType::OTYPE_GOV][ABIL_GUNS];
    ss.sectype = shipdata_secondary(ShipType::OTYPE_GOV);
    ss.max_crew = Shipdata[ShipType::OTYPE_GOV][ABIL_MAXCREW];
    ss.max_destruct = Shipdata[ShipType::OTYPE_GOV][ABIL_DESTCAP];
    ss.max_resource = Shipdata[ShipType::OTYPE_GOV][ABIL_CARGO];
    ss.max_fuel = Shipdata[ShipType::OTYPE_GOV][ABIL_FUELCAP];
    ss.max_speed = Shipdata[ShipType::OTYPE_GOV][ABIL_SPEED];
    ss.build_cost = Shipdata[ShipType::OTYPE_GOV][ABIL_COST];
    ss.size = 100;
    ss.base_mass = 100.0;
    ss.shipclass = "Standard";

    ss.fuel = 0.0;
    ss.popn = Shipdata[ss.type][ABIL_MAXCREW];
    ss.troops = 0;
    ss.mass = ss.base_mass + Shipdata[ss.type][ABIL_MAXCREW] * race->mass;
    ss.destruct = ss.resource = 0;

    ss.alive = 1;
    ss.active = 1;
    ss.protect.self = 1;

    ss.docked = 1;
    /* docked on the planet */
    ss.whatorbits = ScopeLevel::LEVEL_PLAN;
    ss.whatdest = ScopeLevel::LEVEL_PLAN;
    ss.deststar = star;
    ss.destpnum = pnum;
    ss.storbits = star;
    ss.pnumorbits = pnum;
    ss.rad = 0;
    ss.damage = 0; /*Shipdata[ss.type][ABIL_DAMAGE];*/
    /* (first capital is 100% efficient */
    ss.retaliate = 0;

    ss.ships = 0;

    ss.on = 1;

    ss.name[0] = '\0';
    ss.number = shipno;
    Ship s{ss};  // Construct Ship from POD struct

    // Save ship using repository
    ShipRepository ships(store);
    ships.save(s);
  }

  planet.info(Playernum).numsectsowned = 1;
  planet.explored() = 0;
  planet.info(Playernum).explored = 1;

  // (approximate)
  planet.maxpopn() =
      maxsupport(*race, sect, 100.0, 0) * planet.Maxx() * planet.Maxy() / 2;

  // Save race using repository
  RaceRepository races(store);
  races.save(*race);

  // planet_handle and smap_handle will auto-save when they go out of scope

  // Update star
  auto star_handle = entity_manager.get_star(star);
  auto& star_data = *star_handle;
  setbit(star_data.explored(), Playernum);
  setbit(star_data.inhabited(), Playernum);
  star_data.AP(Playernum) = 5;
  // star_handle will auto-save when it goes out of scope

  std::cout << std::format(
      "Player {} ({}) created on sector {},{} on {}/{}.\\n", Playernum,
      race_info.name, sect.get_x(), sect.get_y(), star_data.get_name(),
      star_data.get_planet_name(pnum));
  race_info.status = STATUS_ENROLLED;
  return 0;
}
