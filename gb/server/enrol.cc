// SPDX-License-Identifier: Apache-2.0

/// \file enrol.cc
/// \brief Player race enrollment CLI executable.

import std;
import gb.entities;
import gb.services;
import dallib;
import scnlib;
#undef stdout

#include "gb/server/enroll.h"

namespace GB::enrol {

struct stype {
  bool here;
  int x, y;
  int count;
};

#define RACIAL_TYPES 10

/* racial types (10 racial types ) */
static int Thing[RACIAL_TYPES] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

static double db_Mass[RACIAL_TYPES] = {.1,   .15,  .2,   .125, .125,
                                       .125, .125, .125, .125, .125};
static double db_Birthrate[RACIAL_TYPES] = {0.9, 0.85, 0.8, 0.5,  0.55,
                                            0.6, 0.65, 0.7, 0.75, 0.8};
static int db_Fighters[RACIAL_TYPES] = {9, 10, 11, 2, 3, 4, 5, 6, 7, 8};
static int db_Intelligence[RACIAL_TYPES] = {0,   0,   0,   190, 180,
                                            170, 160, 150, 140, 130};

static double db_Adventurism[RACIAL_TYPES] = {0.89, 0.89, 0.89, .6,  .65,
                                              .7,   .7,   .75,  .75, .8};

static int Min_Sexes[RACIAL_TYPES] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2};
static int Max_Sexes[RACIAL_TYPES] = {1, 1, 1, 2, 2, 4, 4, 4, 4, 4};
static double db_Metabolism[RACIAL_TYPES] = {3.0,  2.7,  2.4, 1.0,  1.15,
                                             1.30, 1.45, 1.6, 1.75, 1.9};

#define RMass(x) (db_Mass[(x)] + .001 * (double)int_rand(-25, 25))
#define Birthrate(x) (db_Birthrate[(x)] + .01 * (double)int_rand(-10, 10))
#define Fighters(x) (db_Fighters[(x)] + int_rand(-1, 1))
#define Intelligence(x) (db_Intelligence[(x)] + int_rand(-10, 10))
#define Adventurism(x) (db_Adventurism[(x)] + 0.01 * (double)int_rand(-10, 10))
#define Sexes(x)                                                               \
  (int_rand(Min_Sexes[(x)], int_rand(Min_Sexes[(x)], Max_Sexes[(x)])))
#define Metabolism(x) (db_Metabolism[(x)] + .01 * (double)int_rand(-15, 15))

}  // namespace GB::enrol

int main() {
  using namespace GB::enrol;

  int pnum = 0;
  int star = 0;
  bool found = false;
  player_t Playernum;
  PlanetType ppref;
  int idx;
  char c;
  struct stype secttypes[SectorType::SEC_WASTED + 1] = {};
  unsigned char not_found[PlanetType::DESERT + 1] = {};  // Zero-initialized

  // Create Database and EntityManager for dependency injection
  Database database{PKGSTATEDIR "gb.db"};
  EntityManager entity_manager{database};

  // Create JsonStore and repositories for new entity creation
  JsonStore store{database};
  RaceRepository races{store};
  ShipRepository ships{store};

  if ((Playernum = player_t{entity_manager.num_races().value + 1}) >=
      player_t{MAXPLAYERS}) {
    std::println(std::cout, "There are already {} players; No more allowed.",
                 MAXPLAYERS - 1);
    return -1;
  }

  std::print("Enter racial type to be created (1-{}):", RACIAL_TYPES);
  std::string input_line;
  std::getline(std::cin, input_line);
  auto idx_result = scn::scan<int>(input_line, "{}");
  if (!idx_result) {
    std::println(std::cerr, "Error: Cannot read input - {}",
                 idx_result.error().msg());
    return -1;
  }
  idx = idx_result->value();

  if (idx <= 0 || idx > RACIAL_TYPES) {
    std::println(std::cout, "Bad racial index.");
    return 1;
  }
  idx = idx - 1;

  const auto* universe_ptr = entity_manager.peek_universe();
  if (!universe_ptr) {
    std::println(std::cerr, "Error: Cannot load universe data");
    return -1;
  }
  const auto& Sdata = *universe_ptr;
  std::println(std::cout, "There is still space for player {}.", Playernum);

  do {
    std::print("\nLive on what type planet:\n     (e)arth, (g)asgiant, (m)ars, "
               "(i)ce, (w)ater, (d)esert, (f)orest? ");
    std::string planet_line;
    std::getline(std::cin, planet_line);
    c = (!planet_line.empty()) ? planet_line[0] : '\0';

    switch (c) {
      case 'w':
        ppref = PlanetType::WATER;
        break;
      case 'e':
        ppref = PlanetType::EARTH;
        break;
      case 'm':
        ppref = PlanetType::MARS;
        break;
      case 'g':
        ppref = PlanetType::GASGIANT;
        break;
      case 'i':
        ppref = PlanetType::ICEBALL;
        break;
      case 'd':
        ppref = PlanetType::DESERT;
        break;
      case 'f':
        ppref = PlanetType::FOREST;
        break;
      default:
        std::println(std::cout, "Oh well.");
        return -1;
    }

    std::println(std::cout, "Looking for type {} planet...",
                 static_cast<int>(ppref));

    /* find first planet of right type */
    found = false;

    auto cand_stars = shuffled_indices(Sdata.numstars);
    auto found_loc = find_suitable_enrol_planet(
        entity_manager, Sdata.numstars, Playernum.value, ppref, cand_stars);
    if (found_loc) {
      star = found_loc->first;
      pnum = found_loc->second;
      found = true;
    }

    if (!found) {
      std::println(std::cout, "planet type not found in any free systems.");
      not_found[ppref] = 1;
      bool all_exhausted = true;
      for (PlanetType pt : all_planet_types) {
        all_exhausted &= (not_found[pt] != 0);
      }
      if (all_exhausted) {
        std::println(std::cout,
                     "Looks like there aren't any free planets left.  bye..");
        return -1;
      }
      std::println(std::cout, "  Try a different one...");
    }

  } while (!found);

  Race race{};

  std::print("\n\tDeity/Guest/Normal (d/g/n) ?");
  std::string deity_line;
  std::getline(std::cin, deity_line);
  c = (!deity_line.empty()) ? deity_line[0] : '\0';

  race.God = (c == 'd');
  race.Guest = (c == 'g');
  race.name = "Unknown";

  // TODO(jeffbailey): What initializes the rest of the governors?
  race.governor[0].money = 0;
  race.governor[0].homelevel = race.governor[0].deflevel =
      ScopeLevel::LEVEL_PLAN;
  race.governor[0].homesystem = race.governor[0].defsystem = star;
  race.governor[0].homeplanetnum = race.governor[0].defplanetnum = pnum;
  /* display options */
  race.governor[0].toggle.highlight = Playernum;
  race.governor[0].toggle.inverse = 1;
  race.governor[0].toggle.color = 0;
  race.governor[0].active = 1;
  std::print("Enter the password for this race:");
  std::string password_line;
  std::getline(std::cin, password_line);
  race.password = password_line;

  std::print("Enter the password for this leader:");
  std::string gov_password_line;
  std::getline(std::cin, gov_password_line);
  race.governor[0].password = gov_password_line;

  /* make conditions preferred by your people set to (more or less)
     those of the planet : higher the concentration of gas, the higher
     percentage difference between planet and race (commented out) */
  // Set race conditions based on chosen planet
  const auto* cond_planet = entity_manager.peek_planet(star, pnum);
  if (cond_planet) {
    for (Conditions c_type : all_atmosphere_conditions) {
      race.conditions[c_type] = cond_planet->conditions(c_type);
    }
  }
  /*+ int_rand( round_rand(-planet->conditions[j]*2.0),
   * round_rand(planet->conditions[j]*2.0) )*/

  for (player_t p = 1; p <= MAXPLAYERS; ++p) {
    /* messages from autoreport, player #1 are decodable */
    if (p == Playernum || Playernum == 1 || race.God) {
      race.translate[p.value - 1] = 100; /* you can talk to own race */
    } else {
      race.translate[p.value - 1] = 1;
    }
  }

  /* assign racial characteristics */
  race.discoveries = {};
  race.tech = 0.0;
  race.morale = 0;
  race.turn = 0;
  race.allied = 0;
  race.atwar = 0;
  char ok_char;
  do {
    race.mass = RMass(idx);
    race.birthrate = Birthrate(idx);
    race.fighters = Fighters(idx);
    if (Thing[idx]) {
      race.IQ = 0;
      race.Metamorph = race.absorb = race.collective_iq = race.pods = true;
    } else {
      race.IQ = Intelligence(idx);
      race.Metamorph = race.absorb = race.collective_iq = race.pods = false;
    }
    race.adventurism = Adventurism(idx);
    race.number_sexes = Sexes(idx);
    race.metabolism = Metabolism(idx);

    std::println(std::cout, "{}", race.Metamorph ? "METAMORPHIC" : "");
    std::println(std::cout, "       Birthrate: {:.3f}", race.birthrate);
    std::println(std::cout, "Fighting ability: {}", race.fighters);
    std::println(std::cout, "              IQ: {}", race.IQ);
    std::println(std::cout, "      Metabolism: {:.2f}", race.metabolism);
    std::println(std::cout, "     Adventurism: {:.2f}", race.adventurism);
    std::println(std::cout, "            Mass: {:.2f}", race.mass);
    std::println(std::cout, " Number of sexes: {} (min req'd for colonization)",
                 race.number_sexes);

    std::print("\n\nLook OK(y/n)?");
    std::string ok_line;
    std::getline(std::cin, ok_line);
    ok_char = (!ok_line.empty()) ? ok_line[0] : '\0';
  } while (ok_char != 'y');

  const auto* planet_ptr = entity_manager.peek_planet(star, pnum);
  if (!planet_ptr) {
    std::println(std::cerr, "Error: Cannot load planet for sector analysis");
    return -1;
  }

  std::println(std::cout,
               "\nChoose a primary sector preference. This race will prefer to "
               "live\non this type of sector.");

  entity_manager.with_sectormap(star, pnum, [&](const SectorMap& smap) {
    for (const Sector& sector : smap.shuffle()) {
      secttypes[sector.get_condition()].count++;
      if (!secttypes[sector.get_condition()].here) {
        secttypes[sector.get_condition()].here = true;
        secttypes[sector.get_condition()].x = sector.get_x();
        secttypes[sector.get_condition()].y = sector.get_y();
      }
    }
    // Temporarily show sectors during selection (no need to persist)
    for (SectorType st : all_sector_types) {
      if (secttypes[st].here) {
        std::println(std::cout, "({:2d}): {} ({}, {}) ({}, {} sectors)", st,
                     get_sector_char(
                         smap.get(Coordinates{secttypes[st].x, secttypes[st].y})
                             .get_condition()),
                     secttypes[st].x, secttypes[st].y, Desnames[st],
                     secttypes[st].count);
      }
    }
  });

  SectorType chosen_sector{};
  bool sector_chosen = false;
  do {
    std::print("\nchoice (enter the number): ");
    std::string choice_line;
    std::getline(std::cin, choice_line);
    auto choice_result = scn::scan<int>(choice_line, "{}");
    if (!choice_result) {
      std::println(std::cerr, "Error: Cannot read input - {}",
                   choice_result.error().msg());
      return -1;
    }
    auto parsed = to_sector_type(choice_result->value());
    if (!parsed || !secttypes[*parsed].here) {
      std::println(std::cout, "There are none of that type here..");
    } else {
      chosen_sector = *parsed;
      sector_chosen = true;
    }
  } while (!sector_chosen);

  race.likesbest = chosen_sector;
  race.likes[chosen_sector] = 1.0;
  race.likes[SectorType::SEC_PLATED] = 1.0;
  race.likes[SectorType::SEC_WASTED] = 0.0;
  std::println(std::cout, "\nEnter compatibilities of other sectors -");
  for (SectorType st : all_sector_types) {
    if (st < SectorType::SEC_PLATED && st != chosen_sector) {
      std::print("{:6s} ({:3d} sectors) :", Desnames[st], secttypes[st].count);
      std::string compat_line;
      std::getline(std::cin, compat_line);
      auto compat_result = scn::scan<int>(compat_line, "{}");
      if (!compat_result) {
        std::println(std::cerr, "Error: Cannot read input - {}",
                     compat_result.error().msg());
        return -1;
      }
      race.likes[st] = static_cast<double>(compat_result->value()) / 100.0;
    }
  }
  std::println(std::cout, "Numraces = {}", entity_manager.num_races());
  Playernum = race.Playernum = player_t{entity_manager.num_races().value + 1};

  /* build a capital ship to run the government */
  {
    ship_struct ss{};  // POD struct for direct initialization
    shipnum_t shipno;

    shipno = ships.next_ship_number();
    std::println(std::cout, "Creating government ship {}...", shipno);
    race.Gov_ship = shipno;

    ss.type = ShipType::OTYPE_GOV;
    entity_manager.with_star(star, [&](const Star& s) {
      entity_manager.with_planet(star, pnum, [&](const Planet& p) {
        ss.xpos = s.xpos() + p.xpos();
        ss.ypos = s.ypos() + p.ypos();
      });
    });
    ss.land_coords =
        Coordinates{secttypes[chosen_sector].x, secttypes[chosen_sector].y};

    ss.owner = Playernum;
    ss.race = Playernum;

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

    ss.popn = Shipdata[ss.type][ABIL_MAXCREW];
    ss.mass = ss.base_mass + Shipdata[ss.type][ABIL_MAXCREW] * race.mass;

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
    /* (first capital is 100% efficient */

    ss.on = 1;

    ss.number = shipno;
    entity_manager.with_star(ss.storbits, [&](const Star& storbit_star) {
      std::println(std::cout, "Created on sector {} on /{}/{}", ss.land_coords,
                   storbit_star.get_name(),
                   storbit_star.get_planet_name(ss.pnumorbits));
    });
    Ship s{ss};  // Construct Ship from POD
    if (!ships.save(s)) {
      std::println(std::cerr, "Error: Failed to save ship to database");
      return -1;
    }
  }

  std::ranges::fill(race.points, 0);

  if (!races.save(race)) {
    std::println(std::cerr, "Error: Failed to save race to database");
    return -1;
  }

  entity_manager.mutate_sectormap(star, pnum, [&](SectorMap& smap) {
    entity_manager.mutate_planet(star, pnum, [&](Planet& planet) {
      auto& sect = smap.get(
          Coordinates{secttypes[chosen_sector].x, secttypes[chosen_sector].y});
      sect.set_owner(Playernum);
      sect.set_race(Playernum);
      sect.set_fert(100);
      sect.set_efficiency_bounded(10);
      sect.set_popn_exact(race.number_sexes);
      sect.set_troops(0);

      planet.info(Playernum).numsectsowned = 1;
      planet.explored() = 0;
      planet.info(Playernum).explored = 1;
      planet.popn() = race.number_sexes;
      planet.troops() = 0;
      planet.maxpopn() =
          maxsupport(race, sect, 100.0, 0) * planet.num_sectors() / 2;
    });
  });

  /* make star explored and stuff */
  entity_manager.mutate_star(star, [&](Star& star_ref) {
    star_ref.mark_explored_by(Playernum);
    star_ref.mark_inhabited_by(Playernum);
    star_ref.AP(Playernum) = 5;
  });

  std::println(std::cout, "\nYou are player {}.\n", Playernum);
  std::println(std::cout, "Your race has been created on sector {},{} on",
               secttypes[chosen_sector].x, secttypes[chosen_sector].y);
  entity_manager.with_star(star, [&](const Star& home_star) {
    std::println(std::cout, "{}/{}.\n", home_star.get_name(),
                 home_star.get_planet_name(pnum));
  });
  return 0;
}
