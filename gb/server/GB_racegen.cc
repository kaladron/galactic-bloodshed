// SPDX-License-Identifier: Apache-2.0

/// \file GB_racegen.cc
/// \brief Helper functions for race generation and enrollment.

import std;
import dallib;
import gblib;

#include "gb/server/racegen.h"

namespace {
constexpr std::array<PlanetType, N_HOME_PLANET_TYPES> planet_translate = {
    PlanetType::EARTH,   PlanetType::FOREST, PlanetType::DESERT,
    PlanetType::WATER,   PlanetType::MARS,   PlanetType::ICEBALL,
    PlanetType::GASGIANT};
}

int enroll_valid_race(Database& database);

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race() {
  Database database{PKGSTATEDIR "gb.db"};
  return enroll_valid_race(database);
}

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race(Database& database) {
  int star;
  int pnum;

  EntityManager entity_manager{database};
  JsonStore store{database};

  auto Playernum = player_t{entity_manager.num_races().value + 1};
  if ((Playernum == player_t{1}) && (race_info.priv_type != P_GOD)) {
    race_info.rejection = "The first race enrolled must have God privileges.\n";
    return 1;
  }
  if (Playernum >= MAXPLAYERS) {
    race_info.rejection = std::format(
        "There are already {} players; No more allowed.\n", MAXPLAYERS - 1);
    race_info.status = EnrollmentStatus::UNENROLLABLE;
    return 1;
  }

  const auto* universe = entity_manager.peek_universe();
  auto numstars = universe->numstars;

  std::cout << std::format("Looking for {}..",
                           planet_print_name[race_info.home_planet_type]);

  auto ppref = planet_translate[race_info.home_planet_type];
  for (int cand_star : shuffled_indices(numstars)) {
    const auto* star_obj = entity_manager.peek_star(cand_star);
    auto numplanets = star_obj->numplanets();

    for (pnum = 0; pnum < numplanets; pnum++) {
      const auto* pl = entity_manager.peek_planet(cand_star, pnum);
      if (pl->type() == ppref) {
        if (pl->popn() == 0) {
          star = cand_star;
          goto found_planet;
        }
      }
    }
  }

  /*
   * If we get here, then we did not find any good planet. */
  std::cout << " failed!\n";
  race_info.rejection =
      std::format("Didn't find any free {}; choose another home planet type.\n",
                  planet_print_name[race_info.home_planet_type]);
  race_info.status = EnrollmentStatus::UNENROLLABLE;
  return 1;

found_planet:
  std::cout << " found!\n";

  Race race{};

  race.Playernum = Playernum;
  race.God = (race_info.priv_type == P_GOD);
  race.Guest = (race_info.priv_type == P_GUEST);
  race.name = race_info.name;
  race.password = race_info.password;

  race.governor[0].password = "0";
  race.governor[0].homelevel = race.governor[0].deflevel =
      ScopeLevel::LEVEL_PLAN;
  race.governor[0].homesystem = race.governor[0].defsystem = star;
  race.governor[0].homeplanetnum = race.governor[0].defplanetnum = pnum;
  /* display options */
  race.governor[0].toggle.highlight = Playernum;
  race.governor[0].toggle.inverse = true;
  race.governor[0].toggle.color = false;
  race.governor[0].active = true;

  entity_manager.with_planet(star, pnum, [&](const Planet& planet) {
    for (auto i = 0; i <= OTHER; i++)
      race.conditions[i] = planet.conditions(static_cast<Conditions>(i));
  });

  for (auto i = 1; i <= MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == Playernum) || (Playernum == 1) || race.God)
      race.translate[i - 1] = 100; /* you can talk to own race */
    else
      race.translate[i - 1] = 1;
  }

  // Assign racial characteristics
  race.absorb = (race_info.attr[ABSORB] != 0.0);
  race.collective_iq = (race_info.attr[COL_IQ] != 0.0);
  race.Metamorph = (race_info.race_type == R_METAMORPH);
  race.pods = (race_info.attr[PODS] != 0.0);

  race.fighters = race_info.attr[FIGHT];
  if (race_info.attr[COL_IQ] == 1.0)
    race.IQ_limit = race_info.attr[A_IQ];
  else
    race.IQ = race_info.attr[A_IQ];
  race.number_sexes = race_info.attr[SEXES];

  race.fertilize = race_info.attr[FERT] * 100;

  race.adventurism = race_info.attr[ADVENT];
  race.birthrate = race_info.attr[BIRTH];
  race.mass = race_info.attr[MASS];
  race.metabolism = race_info.attr[METAB];

  // Assign sector compats and determine a primary sector type.
  for (SectorType st : all_sector_types) {
    race.likes[st] = race_info.compat[st] / 100.0;
    if ((100 == race_info.compat[st]) &&
        (1.0 == planet_compat_cov[race_info.home_planet_type][st]))
      race.likesbest = st;
  }

  Coordinates capital_coords{0, 0};
  entity_manager.with_sectormap(star, pnum, [&](const SectorMap& smap) {
    for (const Sector& current_sect : smap.shuffle()) {
      if (current_sect.get_condition() == race.likesbest) {
        capital_coords = current_sect.coords();
        return;
      }
    }
  });

  race.governors = 0;

  // Build a capital ship to run the government
  {
    ship_struct ss{};  // POD struct for initialization

    auto shipno = shipnum_t{entity_manager.num_ships().value + 1};
    race.Gov_ship = shipno;
    ss.nextship = 0;

    ss.type = ShipType::OTYPE_GOV;
    entity_manager.with_star(star, [&](const Star& s) {
      entity_manager.with_planet(star, pnum, [&](const Planet& p) {
        ss.xpos = s.xpos() + p.xpos();
        ss.ypos = s.ypos() + p.ypos();
      });
    });
    ss.land_coords = capital_coords;

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
    ss.mass = ss.base_mass + Shipdata[ss.type][ABIL_MAXCREW] * race.mass;
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

  entity_manager.mutate_sectormap(star, pnum, [&](SectorMap& smap) {
    entity_manager.mutate_planet(star, pnum, [&](Planet& planet) {
      auto& sect = smap.get(capital_coords);
      sect.set_owner(Playernum);
      sect.set_race(Playernum);
      sect.set_popn_exact(race.number_sexes);
      sect.set_fert(100);
      sect.set_efficiency_bounded(10);
      sect.set_troops(0);

      planet.popn() = race.number_sexes;
      planet.troops() = 0;
      planet.ships() = race.Gov_ship;
      planet.info(Playernum).numsectsowned = 1;
      planet.explored() = 0;
      planet.info(Playernum).explored = 1;

      // (approximate)
      planet.maxpopn() =
          maxsupport(race, sect, 100.0, 0) * planet.num_sectors() / 2;
    });
  });

  // Save race using repository
  RaceRepository races(store);
  races.save(race);

  // Update star
  entity_manager.mutate_star(star, [&](Star& star_data) {
    setbit(star_data.explored(), Playernum);
    setbit(star_data.inhabited(), Playernum);
    star_data.AP(Playernum) = 5;

    std::cout << std::format(
        "Player {} ({}) created on sector {},{} on {}/{}.\n", Playernum,
        race_info.name, capital_coords.x, capital_coords.y,
        star_data.get_name(), star_data.get_planet_name(pnum));
  });

  race_info.status = EnrollmentStatus::ENROLLED;
  return 0;
}
