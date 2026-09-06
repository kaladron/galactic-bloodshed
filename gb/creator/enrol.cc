// SPDX-License-Identifier: Apache-2.0

/// \file enrol.cc
/// \brief Player race enrollment CLI executable.

import std;
import gb.entities;
import gb.services;
import gb.creator;
import dallib;
import scnlib;
#undef stdout

namespace GB::enrol {

struct SectorTypeSummary {
  bool present{false};
  Coordinates coords{};
  int count{0};
};

struct RaceArchetype {
  bool is_metamorphic{false};
  mass_t base_mass{0.125};
  birthrate_t base_birthrate{0.5};
  fighters_t base_fighters{5};
  iq_t base_iq{150};
  adventurism_t base_adventurism{0.7};
  sexes_t min_sexes{2};
  sexes_t max_sexes{4};
  metabolism_t base_metabolism{1.5};

  [[nodiscard]] mass_t sample_mass() const {
    return base_mass + 0.001 * int_rand(-25, 25);
  }
  [[nodiscard]] birthrate_t sample_birthrate() const {
    return base_birthrate + 0.01 * int_rand(-10, 10);
  }
  [[nodiscard]] fighters_t sample_fighters() const {
    int val = static_cast<int>(base_fighters) + int_rand(-1, 1);
    return static_cast<fighters_t>(std::max(0, val));
  }
  [[nodiscard]] iq_t sample_iq() const {
    if (is_metamorphic) {
      return 0;
    }
    return base_iq + int_rand(-10, 10);
  }
  [[nodiscard]] adventurism_t sample_adventurism() const {
    return base_adventurism + 0.01 * int_rand(-10, 10);
  }
  [[nodiscard]] sexes_t sample_sexes() const {
    int max_val =
        int_rand(static_cast<int>(min_sexes), static_cast<int>(max_sexes));
    return static_cast<sexes_t>(int_rand(static_cast<int>(min_sexes), max_val));
  }
  [[nodiscard]] metabolism_t sample_metabolism() const {
    return base_metabolism + 0.01 * int_rand(-15, 15);
  }
};

constexpr std::array<RaceArchetype, 10> race_archetypes = {{
    // 1: Metamorphic predators
    {.is_metamorphic = true,
     .base_mass = 0.1,
     .base_birthrate = 0.9,
     .base_fighters = 9,
     .base_iq = 0,
     .base_adventurism = 0.89,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 3.0},
    // 2: Metamorphic heavyweights
    {.is_metamorphic = true,
     .base_mass = 0.15,
     .base_birthrate = 0.85,
     .base_fighters = 10,
     .base_iq = 0,
     .base_adventurism = 0.89,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 2.7},
    // 3: Metamorphic colossi
    {.is_metamorphic = true,
     .base_mass = 0.2,
     .base_birthrate = 0.8,
     .base_fighters = 11,
     .base_iq = 0,
     .base_adventurism = 0.89,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 2.4},
    // 4: High intelligence, low combat
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.5,
     .base_fighters = 2,
     .base_iq = 190,
     .base_adventurism = 0.6,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.0},
    // 5
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.55,
     .base_fighters = 3,
     .base_iq = 180,
     .base_adventurism = 0.65,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.15},
    // 6
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.6,
     .base_fighters = 4,
     .base_iq = 170,
     .base_adventurism = 0.7,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.30},
    // 7
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.65,
     .base_fighters = 5,
     .base_iq = 160,
     .base_adventurism = 0.7,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.45},
    // 8
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.7,
     .base_fighters = 6,
     .base_iq = 150,
     .base_adventurism = 0.75,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.6},
    // 9
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.75,
     .base_fighters = 7,
     .base_iq = 140,
     .base_adventurism = 0.75,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.75},
    // 10: Balanced military
    {.is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.8,
     .base_fighters = 8,
     .base_iq = 130,
     .base_adventurism = 0.8,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.9},
}};

}  // namespace GB::enrol

int main(int argc, char* argv[]) {
  using namespace GB::enrol;

  std::string db_path = PKGSTATEDIR "gb.db";

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::println(std::cout, "Usage: enrol [options]");
      std::println(std::cout, "");
      std::println(std::cout, "Options:");
      std::println(std::cout,
                   "  -d, --database, --db <path> Path to SQLite database "
                   "(default: {}gb.db)",
                   PKGSTATEDIR);
      std::println(std::cout,
                   "  -h, --help                  Display this help message "
                   "and exit");
      return 0;
    }
    if (arg == "-d" || arg == "--database" || arg == "--db") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Error: Option \"{}\" requires an argument.",
                     arg);
        return 1;
      }
      db_path = argv[++i];
    } else if (arg.starts_with("--database=")) {
      db_path = arg.substr(std::string_view("--database=").size());
    } else if (arg.starts_with("--db=")) {
      db_path = arg.substr(std::string_view("--db=").size());
    } else {
      std::println(std::cerr, "Unknown option \"{}\".", arg);
      std::println(std::cerr,
                   "Usage: enrol [-d|--database|--db <path>] [-h|--help]");
      return 1;
    }
  }

  planetnum_t pnum{0};
  starnum_t star{0};
  bool found = false;
  player_t Playernum;
  PlanetType ppref;
  char c;
  std::array<SectorTypeSummary, SectorType::SEC_WASTED + 1> secttypes{};
  std::set<PlanetType> exhausted_planet_types;

  // Create Database, EntityManager, and EnrollmentService
  Database database{db_path};
  EntityManager entity_manager{database};
  GB::creator::EnrollmentService service{entity_manager, database};

  if ((Playernum = player_t{entity_manager.num_races().value + 1}) >=
      player_t{MAXPLAYERS}) {
    std::println(std::cout, "There are already {} players; No more allowed.",
                 MAXPLAYERS - 1);
    return -1;
  }

  std::print("Enter racial type to be created (1-{}):", race_archetypes.size());
  std::string input_line;
  std::getline(std::cin, input_line);
  auto idx_result = scn::scan<std::size_t>(input_line, "{}");
  if (!idx_result) {
    std::println(std::cerr, "Error: Cannot read input - {}",
                 idx_result.error().msg());
    return -1;
  }
  std::size_t chosen_idx = idx_result->value();

  if (chosen_idx < 1 || chosen_idx > race_archetypes.size()) {
    std::println(std::cout, "Bad racial index.");
    return 1;
  }
  std::size_t idx = chosen_idx - 1;

  const auto* universe_ptr = entity_manager.peek_universe();
  if (!universe_ptr) {
    std::println(std::cerr, "Error: Cannot load universe data");
    return -1;
  }
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

    std::println(std::cout, "Looking for type {} planet...", ppref);

    /* find first planet of right type */
    found = false;

    auto found_loc = service.find_suitable_planet(ppref);
    if (found_loc) {
      star = found_loc->first;
      pnum = found_loc->second;
      found = true;
    }

    if (!found) {
      std::println(std::cout, "planet type not found in any free systems.");
      exhausted_planet_types.insert(ppref);
      if (exhausted_planet_types.size() >= all_planet_types.size()) {
        std::println(std::cout,
                     "Looks like there aren't any free planets left.  bye..");
        return -1;
      }
      std::println(std::cout, "  Try a different one...");
    }

  } while (!found);

  std::print("\n\tDeity/Guest/Normal (d/g/n) ?");
  std::string deity_line;
  std::getline(std::cin, deity_line);
  c = (!deity_line.empty()) ? deity_line[0] : '\0';

  bool is_god = (c == 'd');
  bool is_guest = (c == 'g');

  std::print("Enter the password for this race:");
  std::string password_line;
  std::getline(std::cin, password_line);
  std::string race_password = password_line;

  std::print("Enter the password for this leader:");
  std::string gov_password_line;
  std::getline(std::cin, gov_password_line);
  std::string gov_password = gov_password_line;

  /* assign racial characteristics */
  const auto& archetype = race_archetypes[idx];
  mass_t race_mass{};
  birthrate_t race_birthrate{};
  fighters_t race_fighters{};
  iq_t race_iq{};
  bool race_metamorph = archetype.is_metamorphic;
  bool race_absorb = archetype.is_metamorphic;
  bool race_collective_iq = archetype.is_metamorphic;
  bool race_pods = archetype.is_metamorphic;
  adventurism_t race_adventurism{};
  sexes_t race_sexes{};
  metabolism_t race_metabolism{};

  char ok_char = '\0';
  do {
    race_mass = archetype.sample_mass();
    race_birthrate = archetype.sample_birthrate();
    race_fighters = archetype.sample_fighters();
    race_iq = archetype.sample_iq();
    race_adventurism = archetype.sample_adventurism();
    race_sexes = archetype.sample_sexes();
    race_metabolism = archetype.sample_metabolism();

    std::println(std::cout, "{}", race_metamorph ? "METAMORPHIC" : "");
    std::println(std::cout, "       Birthrate: {:.3f}", race_birthrate);
    std::println(std::cout, "Fighting ability: {}", race_fighters);
    std::println(std::cout, "              IQ: {}", race_iq);
    std::println(std::cout, "      Metabolism: {:.2f}", race_metabolism);
    std::println(std::cout, "     Adventurism: {:.2f}", race_adventurism);
    std::println(std::cout, "            Mass: {:.2f}", race_mass);
    std::println(std::cout, " Number of sexes: {} (min req'd for colonization)",
                 race_sexes);

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

  // Shuffle to randomize initial home sector coordinates across the world
  // so each newly enrolled player doesn't start in the top-left (0,0) corner.
  entity_manager.with_sectormap(star, pnum, [&](const SectorMap& smap) {
    for (const Sector& sector : smap.shuffle()) {
      secttypes[sector.get_condition()].count++;
      if (!secttypes[sector.get_condition()].present) {
        secttypes[sector.get_condition()].present = true;
        secttypes[sector.get_condition()].coords = sector.coords();
      }
    }
    // Temporarily show sectors during selection (no need to persist)
    for (SectorType st : all_sector_types) {
      if (secttypes[st].present) {
        std::println(
            std::cout, "({:2d}): {} ({}, {}) ({}, {} sectors)", st,
            get_sector_char(smap.get(secttypes[st].coords).get_condition()),
            secttypes[st].coords.x, secttypes[st].coords.y, Desnames[st],
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
    if (!parsed || !secttypes[*parsed].present) {
      std::println(std::cout, "There are none of that type here..");
    } else {
      chosen_sector = *parsed;
      sector_chosen = true;
    }
  } while (!sector_chosen);

  std::array<double, SectorType::SEC_WASTED + 1> sector_compat{};
  sector_compat[chosen_sector] = 1.0;
  sector_compat[SectorType::SEC_PLATED] = 1.0;
  sector_compat[SectorType::SEC_WASTED] = 0.0;
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
      sector_compat[st] = compat_result->value() / 100.0;
    }
  }

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Unknown",
      .password = race_password,
      .governor_password = gov_password,
      .home_planet_type = ppref,
      .preferred_sector = chosen_sector,
      .capital_coords = secttypes[chosen_sector].coords,
      .target_planet = std::make_pair(star, pnum),
      .is_god = is_god,
      .is_guest = is_guest,
      .mass = race_mass,
      .birthrate = race_birthrate,
      .fighters = race_fighters,
      .iq = race_iq,
      .metamorph = race_metamorph,
      .absorb = race_absorb,
      .collective_iq = race_collective_iq,
      .pods = race_pods,
      .adventurism = race_adventurism,
      .number_sexes = race_sexes,
      .metabolism = race_metabolism,
      .sector_compatibilities = sector_compat,
      .likesbest = chosen_sector,
  };

  // EnrollmentService handles complete entity setup: creating Race with
  // properly initialized Leader and inactive governors 1..MAXGOVERNORS,
  // configuring capital sector, home planet, Star, and government ship.
  auto result = service.enroll_player(spec);
  if (!result.success) {
    std::println(std::cerr, "Error: Enrollment failed - {}", result.message);
    return -1;
  }

  std::println(std::cout, "\nYou are player {}.\n", result.player_num);
  std::println(std::cout, "Your race has been created on sector {},{} on",
               result.capital_coords.x, result.capital_coords.y);
  entity_manager.with_star(star, [&](const Star& home_star) {
    std::println(std::cout, "{}/{}.\n", home_star.get_name(),
                 home_star.get_planet_name(pnum));
  });
  return 0;
}
