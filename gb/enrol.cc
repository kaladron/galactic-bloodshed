// SPDX-License-Identifier: Apache-2.0

/* enrol.c -- initializes to owner one sector and planet. */

import std;
import gblib;
import dallib;
import scnlib;

#include <sqlite3.h>
#include <strings.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

namespace GB::enrol {

struct stype {
  bool here;
  int x, y;
  int count;
};

#define RACIAL_TYPES 10

// TODO(jeffbailey): Copied from map.c, but they've diverged
static char desshow(const int x, const int y, SectorMap&);

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
#define Sexes(x) \
  (int_rand(Min_Sexes[(x)], int_rand(Min_Sexes[(x)], Max_Sexes[(x)])))
#define Metabolism(x) (db_Metabolism[(x)] + .01 * (double)int_rand(-15, 15))

}  // namespace GB::enrol

int main() {
  using namespace GB::enrol;

  int pnum = 0;
  int star = 0;
  int found = 0;
  int check;
  int vacant;
  int count;
  int i;
  int j;
  player_t Playernum;
  PlanetType ppref;
  int idx;
  int k;
  char c;
  struct stype secttypes[SectorType::SEC_WASTED + 1] = {};
  Planet planet;
  unsigned char not_found[PlanetType::DESERT + 1] = {};  // Zero-initialized
  const Star* selected_star = nullptr;

  // Create Database and EntityManager for dependency injection
  Database database{PKGSTATEDIR "gb.db"};
  EntityManager entity_manager{database};

  srandom(getpid());

  if ((Playernum = entity_manager.num_races() + 1) >= MAXPLAYERS) {
    std::println("There are already {} players; No more allowed.",
                 MAXPLAYERS - 1);
    return -1;
  }

  std::print("Enter racial type to be created (1-{}):", RACIAL_TYPES);
  std::string input_line;
  std::getline(std::cin, input_line);
  auto idx_result = scn::scan<int>(input_line, "{}");
  if (!idx_result) {
    std::println(stderr, "Error: Cannot read input - {}",
                 idx_result.error().msg());
    return -1;
  }
  idx = idx_result->value();

  if (idx <= 0 || idx > RACIAL_TYPES) {
    std::println("Bad racial index.");
    return 1;
  }
  idx = idx - 1;

  const auto* stardata_ptr = entity_manager.peek_stardata();
  if (!stardata_ptr) {
    std::println(stderr, "Error: Cannot load star data");
    return -1;
  }
  const auto& Sdata = *stardata_ptr;
  std::println("There is still space for player {}.", Playernum);

  do {
    std::print(
        "\nLive on what type planet:\n     (e)arth, (g)asgiant, (m)ars, "
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
        std::println("Oh well.");
        return -1;
    }

    std::println("Looking for type {} planet...", static_cast<int>(ppref));

    /* find first planet of right type */
    count = 0;
    found = 0;
    selected_star = nullptr;

    for (star = 0; star < Sdata.numstars && !found && count < 100;) {
      const auto* star_ptr = entity_manager.peek_star(star);
      if (!star_ptr) {
        count++;
        star = int_rand(0, Sdata.numstars - 1);
        continue;
      }

      check = 1;
      /* skip over inhabited stars - or stars with just one planet! */
      if (star_ptr->inhabited() != 0 || star_ptr->numplanets() < 2) check = 0;

      /* look for uninhabited planets */
      if (check) {
        pnum = 0;
        while (!found && pnum < star_ptr->numplanets()) {
          planet = getplanet(star, pnum);

          if (planet.type() == ppref && star_ptr->numplanets() != 1) {
            vacant = 1;
            for (i = 1; i <= Playernum; i++)
              if (planet.info(i - 1).numsectsowned) vacant = 0;
            if (vacant && planet.conditions(RTEMP) >= -50 &&
                planet.conditions(RTEMP) <= 50) {
              found = 1;
              selected_star = star_ptr;
            }
          }
          if (!found) {
            pnum++;
          }
        }
      }

      if (!found) {
        count++;
        star = int_rand(0, Sdata.numstars - 1);
      }
    }

    if (!found) {
      std::println("planet type not found in any free systems.");
      not_found[ppref] = 1;
      for (found = 1, i = PlanetType::EARTH; i <= PlanetType::DESERT; i++)
        found &= not_found[i];
      if (found) {
        std::println("Looks like there aren't any free planets left.  bye..");
        return -1;
      } else
        std::println("  Try a different one...");
      found = 0;
    }

  } while (!found);

  if (!selected_star) {
    std::println(stderr, "Error: Unable to load star data for selection");
    return -1;
  }

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
  for (j = 0; j <= OTHER; j++) race.conditions[j] = planet.conditions(static_cast<Conditions>(j));
  /*+ int_rand( round_rand(-planet->conditions[j]*2.0),
   * round_rand(planet->conditions[j]*2.0) )*/

  for (i = 0; i < MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == (Playernum - 1) || Playernum == 1) || race.God)
      race.translate[i] = 100; /* you can talk to own race */
    else
      race.translate[i] = 1;
  }

  /* assign racial characteristics */
  for (i = 0; i < NUM_DISCOVERIES; i++) race.discoveries[i] = 0;
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

    std::println("{}", race.Metamorph ? "METAMORPHIC" : "");
    std::println("       Birthrate: {:.3f}", race.birthrate);
    std::println("Fighting ability: {}", race.fighters);
    std::println("              IQ: {}", race.IQ);
    std::println("      Metabolism: {:.2f}", race.metabolism);
    std::println("     Adventurism: {:.2f}", race.adventurism);
    std::println("            Mass: {:.2f}", race.mass);
    std::println(" Number of sexes: {} (min req'd for colonization)",
                 race.number_sexes);

    std::print("\n\nLook OK(y/n)?");
    std::string ok_line;
    std::getline(std::cin, ok_line);
    ok_char = (!ok_line.empty()) ? ok_line[0] : '\0';
  } while (ok_char != 'y');

  auto smap = getsmap(planet);

  std::println(
      "\nChoose a primary sector preference. This race will prefer to "
      "live\non this type of sector.");

  for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
    Sector& sector = sector_wrap;
    secttypes[sector.condition].count++;
    if (!secttypes[sector.condition].here) {
      secttypes[sector.condition].here = true;
      secttypes[sector.condition].x = sector.x;
      secttypes[sector.condition].y = sector.y;
    }
  }
  planet.explored() = 1;
  for (i = SectorType::SEC_SEA; i <= SectorType::SEC_WASTED; i++)
    if (secttypes[i].here) {
      std::println("({:2d}): {} ({}, {}) ({}, {} sectors)", i,
                   desshow(secttypes[i].x, secttypes[i].y, smap),
                   secttypes[i].x, secttypes[i].y, Desnames[i],
                   secttypes[i].count);
    }
  planet.explored() = 0;

  found = 0;
  do {
    std::print("\nchoice (enter the number): ");
    std::string choice_line;
    std::getline(std::cin, choice_line);
    auto choice_result = scn::scan<int>(choice_line, "{}");
    if (!choice_result) {
      std::println(stderr, "Error: Cannot read input - {}",
                   choice_result.error().msg());
      return -1;
    }
    i = choice_result->value();

    if (i < SectorType::SEC_SEA || i > SectorType::SEC_WASTED ||
        !secttypes[i].here) {
      std::println("There are none of that type here..");
    } else
      found = 1;
  } while (!found);

  auto& sect = smap.get(secttypes[i].x, secttypes[i].y);
  race.likesbest = i;
  race.likes[i] = 1.0;
  race.likes[SectorType::SEC_PLATED] = 1.0;
  race.likes[SectorType::SEC_WASTED] = 0.0;
  std::println("\nEnter compatibilities of other sectors -");
  for (j = SectorType::SEC_SEA; j < SectorType::SEC_PLATED; j++)
    if (i != j) {
      std::print("{:6s} ({:3d} sectors) :", Desnames[j], secttypes[j].count);
      std::string compat_line;
      std::getline(std::cin, compat_line);
      auto compat_result = scn::scan<int>(compat_line, "{}");
      if (!compat_result) {
        std::println(stderr, "Error: Cannot read input - {}",
                     compat_result.error().msg());
        return -1;
      }
      k = compat_result->value();
      race.likes[j] = (double)k / 100.0;
    }
  std::println("Numraces = {}", entity_manager.num_races());
  Playernum = race.Playernum = entity_manager.num_races() + 1;

  /* build a capital ship to run the government */
  {
    Ship s{};  // Uses default member initializers from Ship struct
    int shipno;

    shipno = Numships() + 1;
    std::println("Creating government ship {}...", shipno);
    race.Gov_ship = shipno;
    planet.ships() = shipno;

    s.type = ShipType::OTYPE_GOV;
    s.xpos = selected_star->xpos() + planet.xpos();
    s.ypos = selected_star->ypos() + planet.ypos();
    s.land_x = (char)secttypes[i].x;
    s.land_y = (char)secttypes[i].y;

    s.owner = Playernum;
    s.race = Playernum;

    s.tech = 100.0;

    s.build_type = ShipType::OTYPE_GOV;
    s.armor = Shipdata[ShipType::OTYPE_GOV][ABIL_ARMOR];
    s.guns = PRIMARY;
    s.primary = Shipdata[ShipType::OTYPE_GOV][ABIL_GUNS];
    s.primtype = shipdata_primary(ShipType::OTYPE_GOV);
    s.secondary = Shipdata[ShipType::OTYPE_GOV][ABIL_GUNS];
    s.sectype = shipdata_secondary(ShipType::OTYPE_GOV);
    s.max_crew = Shipdata[ShipType::OTYPE_GOV][ABIL_MAXCREW];
    s.max_destruct = Shipdata[ShipType::OTYPE_GOV][ABIL_DESTCAP];
    s.max_resource = Shipdata[ShipType::OTYPE_GOV][ABIL_CARGO];
    s.max_fuel = Shipdata[ShipType::OTYPE_GOV][ABIL_FUELCAP];
    s.max_speed = Shipdata[ShipType::OTYPE_GOV][ABIL_SPEED];
    s.build_cost = Shipdata[ShipType::OTYPE_GOV][ABIL_COST];
    s.size = 100;
    s.base_mass = 100.0;
    s.shipclass = "Standard";

    s.popn = Shipdata[s.type][ABIL_MAXCREW];
    s.mass = s.base_mass + Shipdata[s.type][ABIL_MAXCREW] * race.mass;

    s.alive = 1;
    s.active = 1;
    s.protect.self = 1;

    s.docked = 1;
    /* docked on the planet */
    s.whatorbits = ScopeLevel::LEVEL_PLAN;
    s.whatdest = ScopeLevel::LEVEL_PLAN;
    s.deststar = star;
    s.destpnum = pnum;
    s.storbits = star;
    s.pnumorbits = pnum;
    /* (first capital is 100% efficient */

    s.on = 1;

    s.number = shipno;
    if (const auto* storbit_star = entity_manager.peek_star(s.storbits);
        storbit_star) {
      std::println("Created on sector {},{} on /{}/{}", s.land_x, s.land_y,
                   storbit_star->get_name(),
                   storbit_star->get_planet_name(s.pnumorbits));
    } else {
      std::println("Created on sector {},{} on an unknown location", s.land_x,
                   s.land_y);
    }
    putship(s);
  }

  for (j = 0; j < MAXPLAYERS; j++) race.points[j] = 0;

  putrace(race);

  planet.info(Playernum - 1).numsectsowned = 1;
  planet.explored() = 0;
  planet.info(Playernum - 1).explored = 1;
  /*planet->info[Playernum-1].autorep = 1;*/

  sect.owner = Playernum;
  sect.race = Playernum;
  sect.popn = planet.popn() = race.number_sexes;
  sect.fert = 100;
  sect.eff = 10;
  sect.troops = planet.troops() = 0;
  planet.maxpopn() =
      maxsupport(race, sect, 100.0, 0) * planet.Maxx() * planet.Maxy() / 2;
  /* (approximate) */

  putsector(sect, planet, secttypes[i].x, secttypes[i].y);
  putplanet(planet, *selected_star, pnum);

  /* make star explored and stuff */
  auto star_record = getstar(star);
  setbit(star_record.explored(), Playernum);
  setbit(star_record.inhabited(), Playernum);
  star_record.AP(Playernum - 1) = 5;
  putstar(star_record, star);

  std::println("\nYou are player {}.\n", Playernum);
  std::println("Your race has been created on sector {},{} on", secttypes[i].x,
               secttypes[i].y);
  if (const auto* home_star = entity_manager.peek_star(star); home_star) {
    std::println("{}/{}.\n", home_star->get_name(),
                 home_star->get_planet_name(pnum));
  } else {
    std::println("Unknown star/planet.\n");
  }
  return 0;
}

namespace GB::enrol {

static char desshow(const int x, const int y,
                    SectorMap& smap) /* copied from map.c */
{
  const auto& s = smap.get(x, y);

  switch (s.condition) {
    case SectorType::SEC_WASTED:
      return CHAR_WASTED;
    case SectorType::SEC_SEA:
      return CHAR_SEA;
    case SectorType::SEC_LAND:
      return CHAR_LAND;
    case SectorType::SEC_MOUNT:
      return CHAR_MOUNT;
    case SectorType::SEC_GAS:
      return CHAR_GAS;
    case SectorType::SEC_PLATED:
      return CHAR_PLATED;
    case SectorType::SEC_DESERT:
      return CHAR_DESERT;
    case SectorType::SEC_FOREST:
      return CHAR_FOREST;
    case SectorType::SEC_ICE:
      return CHAR_ICE;
    default:
      return ('!');
  }
}

}  // namespace GB::enrol
